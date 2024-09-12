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
}
