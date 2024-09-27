//
// Created by jonas on 2024/9/11.
//

#include "JRenderer.h"

#include "glm/gtc/matrix_transform.hpp"

#include "JShadingPipeline.h"
#include "JShaderProgram.h"
#include "JMathUtils.h"
#include "JParallelWrapper.h"

#include "tbb/parallel_pipeline.h"
#include "tbb/task_arena.h"

#include <mutex>
#include <atomic>
#include <thread>
#include <assimp/mesh.h>

using std::atomic;

namespace JackalRenderer {
    using MutexType = tbb::spin_mutex;//自旋锁, 忙等待
    static constexpr int PIPELINE_BATCH_SIZE = 512; //
    //CPP 11 standard之后，const和constexpr分工明确， const代表只读，而constexpr代表常量表达式，只读并不代表不会被修改
    using FragmentCache = array<vector<JShadingPipeline::QuadFragments>, PIPELINE_BATCH_SIZE>;

    class DrawcallSetting final {
    public:
        //类中const变量可以在构造函数中才初始化，这符合C++的规则
        const JVertexBuffer& vertex_buffer;
        const JIndexBuffer& index_buffer;
        JShadingPipeline* shader_handler;
        const JShadingState& shading_state;
        const glm::mat4& viewport_matrix;
        float near, far;
        JFrameBuffer* frame_buffer;
        explicit DrawcallSetting(
            const JVertexBuffer& vbo,
            const JIndexBuffer& ibo,
            JShadingPipeline* handler,
            const JShadingState& state,
            const glm::mat4& viewportMat,
            float np,
            float fp,
            JFrameBuffer* fbo) : vertex_buffer(vbo), index_buffer(ibo),
        shader_handler(handler), shading_state(state),
        viewport_matrix(viewportMat),near(np), far(fp), frame_buffer(fbo) {}
    };

    class FramebufferMutex final {
    public:
        using MutexBuffer = vector<MutexType*>;
        int _width, _height;
        MutexBuffer mutex_buffer;
        FramebufferMutex(int width, int height) : _width(width), _height(height) {
            //each pixel has a mutex lock
            mutex_buffer.resize(width * height, nullptr);
            for(int i = 0; i < width * height; ++i)
                    mutex_buffer[i] = new MutexType();
        }
        ~FramebufferMutex() {
            for(auto& mutex : mutex_buffer) {
                delete mutex;
                mutex = nullptr;
            }
        }

        MutexType& getLocker(const int& x, const int& y) {
            return *mutex_buffer[y * _width + x];
        }
    };

    class TBBVertexRastFilter final {
    private:
        int batchSize;
        const int startIndex;
        const int overIndex;
        const DrawcallSetting& draw_call;
        static atomic<int> currIndex; //原子变量
        //创建std::atomic<T> XXX;
        //读取T xxx = XXX.load();
        //修改XXX.store(a);
        //原子操作函数: exchange(), compare_exchange_weak(), compare_exchange_strong(), fetch_add(), fetch_sub()
        //ref: https://www.runoob.com/cplusplus/cpp-multithreading.html
        FragmentCache& fragment_cache;
    public:
        explicit TBBVertexRastFilter(int bs, int startIdx, int overIdx, const DrawcallSetting& drawcall, FragmentCache& cache) :
        batchSize(bs), startIndex(startIdx), overIndex(overIdx), draw_call(drawcall), fragment_cache(cache) {
            currIndex.store(startIdx);
        }
        /*
         * tbb::flow_control 是一个类类型，主要用于在某些并行算法中提供提前终止的机制。
         * 通过这个类，用户可以在某些条件下终止循环的执行，而无需遍历所有元素。
         */
        int operator()(tbb::flow_control& fc) const {
            int faceIndex = 0;
            {
                if((faceIndex = currIndex.fetch_add(1) >= overIndex)) {
                    fc.stop();
                    return -1;
                }
            }
            int order = faceIndex - startIndex; //当前面次序
            faceIndex *= 3;

            JShadingPipeline::VertexData v[3];
            const auto& indexBuffer = draw_call.index_buffer;
            const auto& vertexBuffer = draw_call.vertex_buffer;
#pragma unroll 3
            for(int i = 0; i < 3; ++i) {
                v[i].pos = vertexBuffer[indexBuffer[faceIndex + i]].vpostions;
                v[i].nor = vertexBuffer[indexBuffer[faceIndex + i]].vnormals;
                v[i].tex = vertexBuffer[indexBuffer[faceIndex + i]].vtexcoords;
                v[i].tbn[0] = vertexBuffer[indexBuffer[faceIndex + i]].vtangent;
                v[i].tbn[1] = vertexBuffer[indexBuffer[faceIndex + i]].vbitanget;
            }
            draw_call.shader_handler -> vertexShader(v[0]);
            draw_call.shader_handler -> vertexShader(v[1]);
            draw_call.shader_handler -> vertexShader(v[2]);

            vector<JShadingPipeline::VertexData> clipped_vertices;
            clipped_vertices = JRenderer::clipingSutherlandHodgeman(v[0], v[0], v[2], draw_call.near, draw_call.far);
            if(clipped_vertices.empty())
                return -1;

            for(auto& vertex : clipped_vertices) {
                JShadingPipeline::VertexData::prePerspCorrection(vertex);
                vertex.cpos *= vertex.rhw;
            }

            int num_vertices = clipped_vertices.size();
            for(int i = 0; i < num_vertices - 2; ++i) {
                JShadingPipeline::VertexData vertex[3] = {clipped_vertices[0], clipped_vertices[i + 1], clipped_vertices[i + 2]};
                //TODO testing offset glm::vec4(0.5f)
                vertex[0].spos = glm::ivec2(draw_call.viewport_matrix * vertex[0].cpos + glm::vec4(0.5f));
                vertex[1].spos = glm::ivec2(draw_call.viewport_matrix * vertex[1].cpos + glm::vec4(0.5f));
                vertex[2].spos = glm::ivec2(draw_call.viewport_matrix * vertex[2].cpos + glm::vec4(0.5f));

                if(FaceCulling(vertex[0].spos, vertex[1].spos, vertex[2].spos, draw_call.shading_state.cullFaceMode))
                    continue;
                JShadingPipeline::rasterizeFillEdgeFunction(vertex[0], vertex[1], vertex[2], draw_call.frame_buffer -> getWidth(), draw_call.frame_buffer -> getHeight(), fragment_cache[order]);
            }
            return order;
        }

    private:
        static inline bool FaceCulling(const glm::ivec2& v0, const glm::ivec2& v1, const glm::ivec2& v2, JCullFaceMode mode) {
            if(mode == JCullFaceMode::J_CULL_DISABLE)
                return false;
            auto e1 = v1 - v0;
            auto e2 = v2 - v0;
            int orient = e1.x * e2.y - e1.y * e2.x;
            //orient > 0 背面， < 0 正面
            return (mode == JCullFaceMode::J_CULL_BACK) ? orient > 0 : orient < 0;
        }
    };

    atomic<int> TBBVertexRastFilter::currIndex;

    class TBBFragmentFilter final {
    private:
        int batchSize;
        const DrawcallSetting& drawcall_setting_;
        FragmentCache& fragment_cache_;
        FramebufferMutex& framebuffer_mutex_;
    public:
        explicit TBBFragmentFilter(int bs, const DrawcallSetting& drawcall, FragmentCache& cache, FramebufferMutex& fbmutex) :
        batchSize(bs), drawcall_setting_(drawcall), fragment_cache_(cache), framebuffer_mutex_(fbmutex) {}

        void operator()(int idx) const {
            if(idx == -1 || fragment_cache_[idx].empty())
                return;
            auto fragment_func = [&](JShadingPipeline::FragmentData& fragment, const glm::vec2& dUVdx, const glm::vec2& dUVdy) {
                if(fragment.spos.x == -1)
                    return;
                auto& coverage = fragment.coverage;
                const auto& fragCoord = fragment.spos;
                auto& framebuffer = drawcall_setting_.frame_buffer;
                const auto& shadingState = drawcall_setting_.shading_state;
                //防止(x,y)处的深度缓冲被同时访问
                MutexType::scoped_lock lock(framebuffer_mutex_.getLocker(fragCoord.x, fragCoord.y));

                const int samplingNum = JMaskPixelSampler::getSamplingNum();

                int num_failed = 0;
                if(shadingState.depthTestMode == JDepthTestMode::J_DEPTH_TEST_ENABLE) {
                    const auto& coverageDepth = fragment.coverageDepth;
#pragma unroll
                    for(int s = 0; s < samplingNum; ++s) {
                        if(coverage[s] == 1 && framebuffer -> readDepth(fragCoord.x, fragCoord.y, s) >= coverageDepth[s]) {
                            coverage[s] = 0;
                            ++num_failed;
                        }else if(coverage[s] == 0)
                            ++num_failed;
                    }
                }

                glm::vec4 fragColor;
                drawcall_setting_.shader_handler -> fragmentShader(fragment, fragColor, dUVdx, dUVdy);

                if(shadingState.alphaBlendingMode == JAlphaBlendingMode::J_ALPHA_TO_COVERAGE && samplingNum >= 4) {
                    int num_cancle = samplingNum - int(samplingNum * fragColor.a);
                    if(num_cancle == samplingNum)
                        return;
                    for(int c = 0; c < num_cancle; ++c)
                        coverage[c] = 0;
                }

                switch (shadingState.alphaBlendingMode) {
                    case JAlphaBlendingMode::J_ALPHA_DISABLE:

                    case JAlphaBlendingMode::J_ALPHA_TO_COVERAGE:
                        framebuffer -> writeColorWithMask(fragCoord.x, fragCoord.y, fragColor, coverage);
                        break;
                    case JAlphaBlendingMode::J_ALPHA_BLENDING:
                        framebuffer -> writeColorWithMaskAlphaBlending(fragCoord.x, fragCoord.y, fragColor, coverage);
                        break;
                    default:
                        framebuffer -> writeColorWithMask(fragCoord.x, fragCoord.y, fragColor, coverage);
                        break;
                }

                if(shadingState.depthWriteMode == JDepthWriteMode::J_DEPTH_WRITE_ENABLE)
                    framebuffer -> writeDepthWithMask(fragCoord.x, fragCoord.y, fragment.coverageDepth, coverage);
            };

            parallelLoop((size_t)0, (size_t)fragment_cache_[idx].size(), [&](const size_t& f) {
                auto& block = fragment_cache_[idx][f];
                block.aftPerspCorrectionforBlocks();
                glm::vec2 dUVdx(block.dUdx(), block.dVdx());
                glm::vec2 dUVdy(block.dUdy(), block.dVdy());
                fragment_func(block.fragments[0], dUVdx, dUVdy);
                fragment_func(block.fragments[1], dUVdx, dUVdy);
                fragment_func(block.fragments[2], dUVdx, dUVdy);
                fragment_func(block.fragments[3], dUVdx, dUVdy);
            }, JExecutionPolicy::J_PARALLEL);

            fragment_cache_[idx].clear();
        }
    };

    JRenderer::JRenderer(int width, int height) : backBuffer(nullptr), frontBuffer(nullptr){
        backBuffer = std::make_shared<JFrameBuffer>(width, height);
        frontBuffer = std::make_shared<JFrameBuffer>(width, height);
        renderedImg.resize(width * height * 3, 0);
        //ndc space -> screen space
        viewport_Matrix = JMathUtils::calcViewPortMatrix(width, height);
    }

    void JRenderer::addDrawableMesh(JDrawableMesh::ptr mesh) {
        drawable_meshes_.push_back(mesh);
    }
    //for batch
    void JRenderer::addDrawableMesh(const vector<JDrawableMesh::ptr> &meshes) {
        drawable_meshes_.insert(drawable_meshes_.end(), meshes.begin(), meshes.end());
    }

    void JRenderer::unloadDrawableMesh() {
        for(size_t i = 0; i < drawable_meshes_.size(); ++i)
            drawable_meshes_[i] -> clear();
        vector<JDrawableMesh::ptr>().swap(drawable_meshes_);
    }

    void JRenderer::setViewerPos(const glm::vec3 &viewer) {
        if(shaderHandler == nullptr)
            return;
        shaderHandler -> setViewerPos(viewer);
    }
    /*
     * @return the index of current lightSource
     */
    int JRenderer::addLightSource(JLight::ptr lightSource) {
        return JShadingPipeline::addLight(lightSource);
    }

    JLight::ptr JRenderer::getLightSource(const int &idx) {
        return JShadingPipeline::getLight(idx);
    }

    void JRenderer::setExposure(const float &exposure) {
        JShadingPipeline::setExposure(exposure);
    }

    uint JRenderer::renderAllDrawableMeshes() {
        if(shaderHandler == nullptr) {
            shaderHandler = std::make_shared<J3DShadingPipeline>();
        }
        shaderHandler -> setModelMatrix(model_Matrix);
        shaderHandler -> setViewProjectMatrix(project_Matrix * view_Matrix);

        uint numTriangles = 0;
        for(size_t m = 0; m < drawable_meshes_.size(); ++m) {
            numTriangles += renderAllDrawableMesh(m);
        }

        backBuffer -> resolve();
        {
            std::swap(backBuffer, frontBuffer);
        }
        return numTriangles;
    }

    uint JRenderer::renderAllDrawableMesh(const size_t &idx) {
        if(idx >= drawable_meshes_.size())
            return 0;

        uint numTriangles = 0;
        const auto& drawable = drawable_meshes_[idx];
        const auto& submeshes = drawable -> getDrawableSubMeshes();

        shading_state_.cullFaceMode = drawable -> getCullFaceMode();
        shading_state_.depthTestMode = drawable -> getDepthTestMode();
        shading_state_.depthWriteMode = drawable -> getDepthWriteMode();
        shading_state_.alphaBlendingMode = drawable -> getAlphaBlendingMode();

        shaderHandler -> setModelMatrix(drawable -> getModelMatrix());
        shaderHandler -> setLightingEnable(drawable -> getLightingMode() == JLightingMode::J_LIGHTING_ENABLE);
        shaderHandler -> setAmbientCoef(drawable -> getAmbientCoff());
        shaderHandler -> setDiffuseCoef(drawable -> getDiffuseCoff());
        shaderHandler -> setSpecularCoef(drawable -> getSpecularCoff());
        shaderHandler -> setEmissionColor(drawable -> getEmissionCoff());
        shaderHandler -> setShininess(drawable -> getSpecularExponent());
        shaderHandler -> setTransparency(drawable -> getTransparency());

        tbb::filter_mode executeMode = shading_state_.alphaBlendingMode == JAlphaBlendingMode::J_ALPHA_DISABLE ? tbb::filter_mode::parallel : tbb::filter_mode::serial_in_order;

        static int ntokens = tbb::this_task_arena::max_concurrency() * 128;
        static FragmentCache fragment_cache;
        static FramebufferMutex frab_framebuffer_mutex(backBuffer -> getWidth(), backBuffer -> getHeight());
        for(size_t s = 0; s < submeshes.size(); ++s) {
            const auto& submesh = submeshes[s];
            int faceNum = submesh.getIndices().size() / 3;
            numTriangles + faceNum;

            shaderHandler -> setDiffuseTexId(submesh.getDiffuseMapTexId());
            shaderHandler -> setSpecularTexId(submesh.getSpecularMapTexId());
            shaderHandler -> setNormalTexId(submesh.getNormalMapTexId());
            shaderHandler -> setGlowTexId(submesh.getGlowMapTexId()); //Emission

            DrawcallSetting drawCall(submesh.getVertices(), submesh.getIndices(), shaderHandler.get(),
                shading_state_, viewport_Matrix, frustumNearFar.x, frustumNearFar.y, backBuffer.get());

            for(int f = 0; f < faceNum; f += PIPELINE_BATCH_SIZE) {
                int startIdx = f;
                int endIdx = glm::min(f + PIPELINE_BATCH_SIZE, faceNum);
                tbb::parallel_pipeline(ntokens, tbb::make_filter<void, int>(executeMode, TBBVertexRastFilter(PIPELINE_BATCH_SIZE, startIdx, endIdx, drawCall, fragment_cache)) &
                    tbb::make_filter<int, void>(executeMode, TBBFragmentFilter(PIPELINE_BATCH_SIZE, drawCall, fragment_cache, frab_framebuffer_mutex)));
            }
        }
        return numTriangles;
    }

    uchar *JRenderer::commitRenderedColorBuffer() {
        const auto& pixelBuffer = frontBuffer -> getColorBuffer();
        parallelLoop((size_t)0, (size_t)(frontBuffer -> getWidth() * frontBuffer -> getHeight()), [&](const size_t& index) {
            const auto& pixel = pixelBuffer[index];
            renderedImg[index * 3 + 0] = pixel[0][0];
            renderedImg[index * 3 + 1] = pixel[0][1];
            renderedImg[index * 3 + 2] = pixel[0][2];
        });
        return renderedImg.data();
    }

    vector<JShadingPipeline::VertexData> JRenderer::clipingSutherlandHodgeman(const JShadingPipeline::VertexData &v0, const JShadingPipeline::VertexData &v1, const JShadingPipeline::VertexData &v2, const float &near, const float &far) {
        //clipping using homogeneous coordinates
        //ref: https://dl.acm.org/doi/pdf/10.1145/965139.807398
        auto isPointInsideInClipingFrustum = [](const glm::vec4& p, const float& near, const float& far) -> bool {
            return (p.x <= p.w && p.x >= -p.w) && (p.y <= p.w && p.y >= -p.w)
                && (p.z <= p.w && p.z >= -p.w) && (p.w <= far && p.w >= near);
        };

        if(isPointInsideInClipingFrustum(v0.cpos, near, far) &&
            isPointInsideInClipingFrustum(v1.cpos, near, far) &&
            isPointInsideInClipingFrustum(v2.cpos, near, far)) {
            return {v0, v1, v2};
        }// all vertices are inside the frustum

        //all vertices are outside the frustum, faster than below
        if (v0.cpos.w < near && v1.cpos.w < near && v2.cpos.w < near)
            return{};
        if (v0.cpos.w > far && v1.cpos.w > far && v2.cpos.w > far)
            return{};
        if (v0.cpos.x > v0.cpos.w && v1.cpos.x > v1.cpos.w && v2.cpos.x > v2.cpos.w)
            return{};
        if (v0.cpos.x < -v0.cpos.w && v1.cpos.x < -v1.cpos.w && v2.cpos.x < -v2.cpos.w)
            return{};
        if (v0.cpos.y > v0.cpos.w && v1.cpos.y > v1.cpos.w && v2.cpos.y > v2.cpos.w)
            return{};
        if (v0.cpos.y < -v0.cpos.w && v1.cpos.y < -v1.cpos.w && v2.cpos.y < -v2.cpos.w)
            return{};
        if (v0.cpos.z > v0.cpos.w && v1.cpos.z > v1.cpos.w && v2.cpos.z > v2.cpos.w)
            return{};
        if (v0.cpos.z < -v0.cpos.w && v1.cpos.z < -v1.cpos.w && v2.cpos.z < -v2.cpos.w)
            return{};

        // {
        //     //as alternative
        //     if(!isPointInsideInClipingFrustum(v0.cpos, near, far) &&
        //         !isPointInsideInClipingFrustum(v1.cpos, near, far) &&
        //         !isPointInsideInClipingFrustum(v2.cpos, near, far)) {
        //         return {};
        //     }
        // }


        vector<JShadingPipeline::VertexData> insideVertices;
        vector<JShadingPipeline::VertexData> tmp = {v0, v1, v2}; //原始顶点, 迭代初始值
        enum Axis { X = 0, Y = 1, Z = 2};

        //3d clipping
        {
            insideVertices = clipingSutherlandHodgemanAux(tmp, Axis::X, +1);
            tmp = insideVertices;
            insideVertices = clipingSutherlandHodgemanAux(tmp, Axis::X, -1);
            tmp = insideVertices;
        }
        {
            insideVertices = clipingSutherlandHodgemanAux(tmp, Axis::Y, +1);
            tmp = insideVertices;
            insideVertices = clipingSutherlandHodgemanAux(tmp, Axis::Y, -1);
            tmp = insideVertices;
        }
        {
            insideVertices = clipingSutherlandHodgemanAux(tmp, Axis::Z, +1);
            tmp = insideVertices;
            insideVertices = clipingSutherlandHodgemanAux(tmp, Axis::Z, -1);
            tmp = insideVertices;
        }

        {
            insideVertices = {};
            int numVerts = tmp.size();
            constexpr float wClippingPlane = 1e-5;
            for(int i = 0; i < numVerts; ++i) {
                const auto& begVert = tmp[(i - 1 + numVerts) % numVerts];
                const auto& endVert = tmp[i];
                float begIsInside = (begVert.cpos.w < wClippingPlane) ? -1 : 1;
                float endIsInside = (endVert.cpos.w < wClippingPlane) ? -1 : 1;
                if(begIsInside * endIsInside < 0) {
                    float t = (wClippingPlane - begVert.cpos.w) / (begVert.cpos.w - endVert.cpos.w);
                    auto intersectedVert = JShadingPipeline::VertexData::lerp(begVert, endVert, t);
                    insideVertices.push_back(intersectedVert);
                }
                if(endIsInside > 0) {
                    insideVertices.push_back(endVert);
                }
            }
        }
        return insideVertices;
    }

    vector<JShadingPipeline::VertexData> JRenderer::clipingSutherlandHodgemanAux(const vector<JShadingPipeline::VertexData> &polygon, const int &axis, const int &side) {
        vector<JShadingPipeline::VertexData> insidePolygon; //空
        int numVerts = polygon.size();
        for(int i = 0; i < numVerts; ++i) {
            const auto& begVert = polygon[(i - 1 + numVerts) % numVerts];
            const auto& endVert = polygon[i];
            char begIsInside = ((side * (begVert.cpos[axis]) <= begVert.cpos.w) ? 1 : -1);
            char endIsInside = ((side * (endVert.cpos[axis]) <= endVert.cpos.w) ? 1 : -1);
            if(begIsInside * endIsInside < 0) { //有交点
                //TODO fomular
                //float t = (begVert.cpos.w + side * begVert.cpos[axis]) / ((begVert.cpos.w + side * begVert.cpos[axis]) - (endVert.cpos.w + side * endVert.cpos[axis]));
                float t = (begVert.cpos.w - side * begVert.cpos[axis]) / ((begVert.cpos.w - side * begVert.cpos[axis]) - (endVert.cpos.w - side * endVert.cpos[axis]));
                auto intersectedVert = JShadingPipeline::VertexData::lerp(begVert, endVert, t);
                insidePolygon.push_back(intersectedVert);
            }
            if(endIsInside > 0) {
                insidePolygon.push_back(endVert);
            }
            return insidePolygon;
        }
    }
}
