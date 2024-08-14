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

            static void aftPrespCorrection(FragmentData &v);
        };

        class QuadFragments {
        public:
            FragmentData fragments[4];
            inline float dUdx() const {return };

        };
    };
}

#endif //JSHADINGPIPELINE_H
