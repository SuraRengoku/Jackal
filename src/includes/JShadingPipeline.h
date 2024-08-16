//
// Created by jonas on 2024/8/12.
//

#ifndef JSHADINGPIPELINE_H
#define JSHADINGPIPELINE_H

#include <algorithm>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "JLight.h"
#include "JTexture2D.h"
#include "JPixelSampler.h"

namespace JackalRenderer {
    class JShadingPipeline {
    public:
        using ptr = std::shared_ptr<JShadingPipeline>;
        struct FragmentData;
        struct VertexData {
            glm::vec3 pos;
            glm::vec3 nor;
            glm::vec2 tex;
            glm::vec4 cpos;
            glm::ivec2 spos;
            glm::mat3 tbn;
            bool needInterpolatedTBN = false;
            float rhw;
            VertexData() = delete;
            VertexData(const glm::ivec2 &screenPos) : spos(screenPos) {}
            //linear interpolation
            //about lerp: https://en.wikipedia.org/wiki/Linear_interpolation
            static VertexData lerp(const VertexData &v0, const VertexData &v1, float frac);
            static FragmentData barycentricLerp(const VertexData &v0, const VertexData &v1, const VertexData &v2, const glm::vec3 &w);
            static float barycentricLerp(const float &d0, const float &d1, const float &d2, const glm::vec3 &w);

            //perspective correction for interpolation
            static void perPerspCorrection(VertexData &v);
        };

        struct FragmentData {
            glm::vec3 pos;
            glm::vec3 nor;
            glm::vec2 tex;
            glm::ivec2 spos;
            glm::mat3 tbn;
            float rhw;
            JMaskPixelSampler coverage = 0;
            JDepthPixelSampler coverageDepth = 0.0f;

            FragmentData() = delete;
            FragmentData(const glm::ivec2 &screenPos) : spos(screenPos) {}

            static void aftPerspCorrection(FragmentData &v);
        };

        class QuadFragments {
        public:
            FragmentData fragments[4];
            inline float dUdx() const { return fragments[1].tex.x - fragments[0].tex.x; }
            inline float dUdy() const { return fragments[2].tex.x - fragments[0].tex.x; }
            inline float dVdx() const { return fragments[1].tex.y - fragments[0].tex.y; }
            inline float dVdy() const { return fragments[2].tex.y - fragments[0].tex.y; }

            inline void aftPerspCorrectionforBlocks() {
                JShadingPipeline::FragmentData::aftPerspCorrection(fragments[0]);
                JShadingPipeline::FragmentData::aftPerspCorrection(fragments[1]);
                JShadingPipeline::FragmentData::aftPerspCorrection(fragments[2]);
                JShadingPipeline::FragmentData::aftPerspCorrection(fragments[3]);
            }
        };

        virtual ~JShadingPipeline() = default;

        void setModelMatrix(const glm::mat4& model) {
            //TODO
        }

        void setViewProjectMatrix(const glm::mat4& vp) {  }
        void setLightingEnable(bool enable) {  }

        void setAmbientCoef(const glm::vec3& ka) {}
        void setDiffuseCoef(const glm::vec3& kd) {}
        void setSpecularCoef(const glm::vec3& ks) {}
        void setEmissionColor(const glm::vec3& ke) {}
    };
}

#endif //JSHADINGPIPELINE_H
