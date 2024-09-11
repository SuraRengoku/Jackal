//
// Created by jonas on 2024/8/12.
//
#include "JShadingPipeline.h"

#include <glm/gtc/constants.hpp>

#include "JParallelWrapper.h"

namespace JackalRenderer {
    JShadingPipeline::VertexData JShadingPipeline::VertexData::lerp(
        const JShadingPipeline::VertexData& v0,
        const JShadingPipeline::VertexData& v1,
        float frac){
        //线性插值实现
        VertexData result;
        result.pos = (1.0f - frac) * v0.pos + frac * v1.pos;
        result.nor = (1.0f - frac) * v0.nor + frac * v1.nor;
        result.tex = (1.0f - frac) * v0.tex + frac * v1.tex;
        result.cpos = (1.0f - frac) * v0.cpos + frac * v1.cpos;
        // result.spos = (1.0f - frac) * v0.spos + frac * v1.spos; //need reload operator *
        result.spos.x = static_cast<int>((1.0f - frac) * v0.spos.x + frac * v1.spos.x);
        result.spos.y = static_cast<int>((1.0f - frac) * v0.spos.y + frac * v1.spos.y);
        result.rhw = (1.0f - frac) * v0.rhw + frac * v1.rhw;
        if(v0.needInterpolatedTBN) {
            result.tbn = (1.0f - frac) * v0.tbn + frac * v1.tbn;
            result.needInterpolatedTBN = true;
        }
        return result;
    }

    /*
     * @param v0, v1, v2 vertices of triangle
     * @param barycentric coordinates
     */
    JShadingPipeline::FragmentData JShadingPipeline::VertexData::barycentricLerp(
        const VertexData &v0,
        const VertexData &v1,
        const VertexData &v2,
        const glm::vec3 &w) {
        FragmentData result;
        result.pos = w.x * v0.pos + w.y * v1.pos + w.z * v2.pos;
        result.nor = w.x * v0.nor + w.y * v1.nor + w.z * v2.nor;
        result.tex = w.x * v0.tex + w.y * v1.tex + w.z * v2.tex;
        result.rhw = w.x * v0.rhw + w.y * v1.rhw + w.z * v2.rhw;
        result.spos.x = w.x * v0.spos.x + w.y * v1.spos.x + w.z * v2.spos.x;
        result.spos.y = w.x * v0.spos.y + w.y * v1.spos.y + w.z * v2.spos.y;
        if(v0.needInterpolatedTBN)
            result.tbn = w.x * v0.tbn + w.y * v1.tbn + w.z * v2.tbn;
        return result;
    }

    float JShadingPipeline::VertexData::barycentricLerp(
        const float &d0,
        const float &d1,
        const float &d2,
        const glm::vec3 &w) {
        return w.x * d0 + w.y * d1 + w.z * d2;
    }

    void JShadingPipeline::VertexData::prePerspCorrection(VertexData& v) {
        v.rhw *= 1.0f / v.cpos.w;
        v.pos *= v.rhw;
        v.tex *= v.rhw;
        v.nor *= v.rhw;
    }

    void JShadingPipeline::FragmentData::aftPerspCorrection(FragmentData &v) {
        float w = 1.0f / v.rhw;
        v.pos *= w;
        v.tex *= w;
        v.nor *= w;
    }

    std::vector<JTexture2D::ptr> JShadingPipeline::globalTextureUnits = {};
    std::vector<JLight::ptr> JShadingPipeline::lights = {};
    glm::vec3 JShadingPipeline::viewerPos = glm::vec3(0.0f);
    float JShadingPipeline::exposure = 1.0f;

    void JShadingPipeline::rasterizeFillEdgeFunction(
        const VertexData& v0,
        const VertexData& v1,
        const VertexData& v2,
        const uint& screenWidth,
        const uint& screenHeight,
        vector<QuadFragments>& rasterized_points) {

        VertexData v[] = {v0, v1, v2};
        glm::ivec2 boundingMin;
        glm::ivec2 boundingMax;
        boundingMin.x = std::max(std::min(v0.spos.x, std::min(v1.spos.x, v2.spos.x)), 0);
        boundingMin.y = std::max(std::min(v0.spos.y, std::min(v1.spos.y, v2.spos.y)), 0);
        boundingMax.x = std::min(std::max(v0.spos.x, std::max(v1.spos.x, v2.spos.x)), (int)screenWidth - 1);
        boundingMax.y = std::min(std::max(v0.spos.y, std::max(v1.spos.y, v2.spos.y)), (int)screenHeight - 1);

        {//make sure the order of vertices are CCW
            auto e1(v1.spos - v0.spos);
            auto e2(v2.spos - v0.spos);
            int orient = e1.x * e2.y - e1.y * e2.x;
            if(orient > 0)
                std::swap(v[1], v[2]);
        }

        // 3 vertices of the triangle in screen space
        const glm::ivec2& A = v[0].spos;
        const glm::ivec2& B = v[1].spos;
        const glm::ivec2& C = v[2].spos;

        const int I01 = A.x - B.y, I12 = B.y - C.y, I20 = C.y - A.y;
        const int J01 = B.x - A.x, J12 = C.x - B.x, J20 = A.x - C.x;
        const int K01 = A.x * B.y - A.y * B.x;
        const int K12 = B.x * C.y - B.y * C.x;
        const int K20 = C.x * A.y - C.y * A.x;

        int F01 = I01 * boundingMin.x + J01 * boundingMin.y + K01;
        int F12 = I12 * boundingMin.x + J12 * boundingMin.y + K12;
        int F20 = I20 * boundingMin.x + J20 * boundingMin.y + K20;

        //三角形两个顶点或三个顶点坍缩在一起，此时三角形没有实际面积可以渲染
        if(F01 + F12 + F20 == 0)
            return;

        //指定预留内存空间
        rasterized_points.reserve((boundingMax.y - boundingMin.y) * (boundingMax.x - boundingMax.x));

        /* offset：根据 JMaskPixelSampler::getSamplingNum() 返回的采样点数来设置偏移量。
         * 如果采样点数 >= 4，则 offset 为 0；否则 offset 为 1。通常情况下，采样点数越多，精度越高，偏移量可以设为 0；
         * 当采样点较少时，需要增加偏移量来补偿采样不足，避免采样错误。
        */
        const float offset = JMaskPixelSampler::getSamplingNum() >= 4 ? 0.0f : +1.0f;
        const int E1_t = (((B.y > A.y) || (A.y == B.y && A.x < B.x)) ? 0 : offset);
        const int E2_t = (((C.y > B.y) || (B.y == C.y && B.x < C.x)) ? 0 : offset);
        const int E3_t = (((A.y > C.y) || (C.y == A.y && C.x < A.x)) ? 0 : offset);

        int Cy1 = F01, Cy2 = F12, Cy3 = F20;
        const float one_div_delta = 1.0f / (F01 + F12 + F20);
        //F01 + F12 + F20 实际上是三角形面积的两倍

        /*
         * @brief 计算屏幕坐标对应三角形重心坐标
         * @param x, y screen coordinates
        */
        auto barycentricWeight = [&](const float &x, const float &y) -> glm::vec3 {
            glm::vec3 s[2];
            s[0] = glm::vec3(v[2].spos.x - v[0].spos.x, v[1].spos.x - v[0].spos.x, v[0].spos.x - x);
            s[1] = glm::vec3(v[2].spos.y - v[0].spos.y, v[1].spos.y - v[0].spos.y, v[0].spos.y - y);
            auto uf = glm::cross(s[0], s[1]);
            //讨论二维三角形，如果u的z分量不是1则说明P点不在三角形内
            return glm::vec3(1.0f - (uf.x + uf.y) / uf.z, uf.y / uf.z, uf.x / uf.z); //uf.z != 0
        };

        auto sampling_is_inside = [&](const int& x, const int& y, const int& Cx1, const int& Cx2, const int& Cx3, FragmentData& p) -> bool {
            //Invalid, not in the boundingbox
            if(x > boundingMax.x || y > boundingMax.y) {
                p.spos = glm::ivec2(-1);//-1表示像素无效
                return false;
            }
            bool atLeastOneInside = false;//至少一个采样点在三角形内
            const int samplingNum = JMaskPixelSampler::getSamplingNum();
            auto samplingOffsetArray = JMaskPixelSampler::getSamplingOffsets(); //一个像素点， 四个采样点
#pragma unroll //expand the loop, optimize the performance
            for (int s = 0; s < samplingNum; ++s) {
                const auto &offset = samplingOffsetArray[s];
                //Edge function
                const float E1 = Cx1 + offset.x * I01 + offset.y * J01;
                const float E2 = Cx2 + offset.x * I12 + offset.y * J12;
                const float E3 = Cx3 + offset.x * I20 + offset.y + J20;
                //make sure CCW / counter clockwise
                if((E1 + E1_t) <= 0 && (E2 + E2_t) <= 0 && (E3 + E3_t) <= 0) {
                    atLeastOneInside = true;
                    p.coverage[s] = 1; // 第s个采样点在三角形内
                    glm::vec3 uvw = glm::vec3(E2, E3, E1) * one_div_delta;
                    p.coverageDepth[s] = VertexData::barycentricLerp(v[0].rhw, v[1].rhw, v[2].rhw, uvw);
                }
            }

            if(!atLeastOneInside) //所有采样点点都不在三角形内部，丢弃像素
                p.spos = glm::ivec2(-1);
            return atLeastOneInside;
        };

        for(int y = boundingMin.y; y <= boundingMax.y; y += 2) {
            int Cx1 = Cy1, Cx2 = Cy2, Cx3 = Cy3;
#pragma unroll 4
            for(int x = boundingMin.x; x <= boundingMax.x; x += 2) {
                //no adaptive method but just 2 x 2 block based
                QuadFragments group; // 四个像素点， 一个block
                bool inside0 = sampling_is_inside(x, y, Cx1, Cx2, Cx3, group.fragments[0]);
                bool inside1 = sampling_is_inside(x + 1, y, Cx1 + I01, Cx2 + I12, Cx3 + I20, group.fragments[1]);
                bool inside2 = sampling_is_inside(x, y + 1, Cx1 + J01, Cx2 + J12, Cx2 + J20, group.fragments[2]);
                bool inside3 = sampling_is_inside(x + 1, y + 1, Cx1 + I01 + J01, Cx2 + I12 + J12, Cx3 + I20 + J20, group.fragments[3]);
                //至少一个采样点在三角形中
                if(inside0 || inside1 || inside2 || inside3) {
                    if(!inside0) {
                        group.fragments[0] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentricWeight(x, y));
                        group.fragments[0].spos = glm::ivec2(-1);//无效置-1
                    }else {
                        glm::vec3 uvw(Cx2, Cx3, Cx1);
                        auto coverage = group.fragments[0].coverage;
                        auto coverage_depth = group.fragments[0].coverageDepth;
                        group.fragments[0] = VertexData::barycentricLerp(v[0], v[1], v[2], uvw * one_div_delta);
                        //现场还原
                        group.fragments[0].spos = glm::ivec2(x, y);
                        group.fragments[0].coverage = coverage;
                        group.fragments[0].coverageDepth = coverage_depth;
                    }

                    if(!inside1) {
                        group.fragments[1] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentricWeight(x + 1, y));
                        group.fragments[1].spos = glm::ivec2(-1);
                    }else {
                        glm::vec3 uvw(Cx2 + I12, Cx3 + I20, Cx1 + I01);
                        auto coverage = group.fragments[1].coverage;
                        auto coverage_depth = group.fragments[1].coverageDepth;
                        group.fragments[1] = VertexData::barycentricLerp(v[0], v[1], v[2], uvw * one_div_delta);
                        group.fragments[1].spos = glm::ivec2(x + 1, y);
                        group.fragments[1].coverage = coverage;
                        group.fragments[1].coverageDepth = coverage_depth;
                    }

                    if(!inside2) {
                        group.fragments[2] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentricWeight(x, y + 1));
                        group.fragments[2].spos = glm::ivec2(-1);
                    }else {
                        glm::vec3 uvw(Cx2 + J12, Cx3 + J20, Cx1 + J01);
                        auto coverage = group.fragments[2].coverage;
                        auto coverage_depth = group.fragments[2].coverageDepth;
                        group.fragments[2] = VertexData::barycentricLerp(v[0], v[1], v[2], uvw * one_div_delta);
                        group.fragments[2].spos = glm::ivec2(x, y + 1);
                        group.fragments[2].coverage = coverage;
                        group.fragments[2].coverageDepth = coverage_depth;
                    }

                    if(!inside3) {
                        group.fragments[3] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentricWeight(x + 1, y + 1));
                        group.fragments[3].spos = glm::ivec2(-1);
                    }else {
                        glm::vec3 uvw(Cx2 + I12 + J12, Cx3 + I20 + J20, Cx1 + I01 + J01);
                        auto coverage = group.fragments[3].coverage;
                        auto coverage_depth = group.fragments[3].coverageDepth;
                        group.fragments[3] = VertexData::barycentricLerp(v[0], v[1], v[2], uvw * one_div_delta);
                        group.fragments[3].spos = glm::ivec2(x + 1, y + 1);
                        group.fragments[3].coverage = coverage;
                        group.fragments[3].coverageDepth = coverage_depth;
                    }
                    rasterized_points.push_back(group);
                }
                Cx1 += 2 * I01;
                Cx2 += 2 * I12;
                Cx3 += 2 * I20;
            }
            Cy1 += 2 * J01;
            Cy2 += 2 * J12;
            Cy3 += 2 * J20;
        }
    }

    int JShadingPipeline::uploadTexture2D(JTexture2D::ptr tex) {
        if(tex != nullptr) {
            globalTextureUnits.push_back(tex);
            return globalTextureUnits.size() - 1;
        }
        return -1;
    }

    JTexture2D::ptr JShadingPipeline::getTexture2D(int idx) {
        if(idx < 0 || idx >= globalTextureUnits.size())
            return nullptr;
        return globalTextureUnits[idx];
    }

    int JShadingPipeline::addLight(JLight::ptr lightSource) {
        if(lightSource != nullptr) {
            lights.push_back(lightSource);
            return  lights.size() - 1;
        }
        return -1;
    }

    JLight::ptr JShadingPipeline::getLight(int idx) {
        if(idx < 0 || idx >= lights.size())
            return nullptr;
        return lights[idx];
    }

    /*
     * @param idx 纹理编号
     * @param uv 纹理坐标
     * @param dUVdx, dUVdy UV坐标相对于屏幕空间x和y方向的导数，主要用于计算MipMap的选择
     */
    glm::vec4 JShadingPipeline::texture2D(const uint& idx, const glm::vec2& uv, const glm::vec2& dUVdx, const glm::vec2 &dUVdy) {
        if(idx < 0 || idx >= globalTextureUnits.size())
            return glm::vec4(0.0f);//采样失败
        const auto& texture = globalTextureUnits[idx];
        if(texture -> isGeneratedMipmap()) {
            glm::vec2 dfdx = dUVdx * glm::vec2(texture -> getWidth(), texture -> getHeight());
            glm::vec2 dfdy = dUVdy * glm::vec2(texture -> getWidth(), texture -> getHeight());
            float L = glm::max(glm::dot(dfdx, dfdx), glm::dot(dfdy, dfdy));
            //LOD Level of Detail 细节级别，公式为0.5f * log2(L)
            return texture -> sample(uv, glm::max(0.5f * glm::log2(L), 0.0f));
        }else
            return texture -> sample(uv);
    }
}
