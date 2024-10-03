//
// Created by jonas on 2024/8/12.
//

#ifndef JSHADINGPIPELINE_H
#define JSHADINGPIPELINE_H

#include <algorithm>
#include <memory>
#include <vector>
#include "glm/glm.hpp"
#include <functional>
#include "JLight.h"
#include "JTexture2D.h"
#include "JPixelSampler.h"

using std::vector;

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
            float rhw; //Reciprocal Homogeneous W, 即w分量的倒数 1/w
            VertexData() = default;
            VertexData(const glm::ivec2 &screenPos) : spos(screenPos) {}
            //linear interpolation
            //about lerp: https://en.wikipedia.org/wiki/Linear_interpolation
            static VertexData lerp(const VertexData &v0, const VertexData &v1, float frac);
            static FragmentData barycentricLerp(const VertexData &v0, const VertexData &v1, const VertexData &v2, const glm::vec3 &w);
            static float barycentricLerp(const float &d0, const float &d1, const float &d2, const glm::vec3 &w);

            //perspective correction for interpolation
            static void prePerspCorrection(VertexData &v);
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

            FragmentData() = default;
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
            modelMatrix = model;
            inveTransModelMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));//法线矫正矩阵
        }

        void setViewProjectMatrix(const glm::mat4& vp) { viewProjectMatrix = vp; }
        void setLightingEnable(bool enable) { lightingEnable = enable; }

        void setAmbientCoef(const glm::vec3& ka) { kA = ka; }
        void setDiffuseCoef(const glm::vec3& kd) { kD = kd; }
        void setSpecularCoef(const glm::vec3& ks) { kS = ks; }
        void setEmissionColor(const glm::vec3& ke) { kE = ke; }
        void setTransparency(const float &alpha) { transparency = alpha; }
        void setDiffuseTexId(const int& id) { diffuseTexId = id; }
        void setSpecularTexId(const int& id) { specularTexId = id; }
        void setNormalTexId(const int& id) { normalTexId = id; }
        void setGlowTexId(const int& id) { glowTexId = id; }
        void setShininess(const float& shininess) { this -> shininess = shininess; }


        virtual void vertexShader(VertexData& vertex) const = 0;
        virtual void fragmentShader(const FragmentData& data, glm::vec4& fragColor, const glm::vec2& dUVdx, const glm::vec2& dUVdy) const = 0;

        static void rasterizeFillEdgeFunction(
            const VertexData& v0,
            const VertexData& v1,
            const VertexData& v2,
            const uint& screenWidth,
            const uint& screenHeight,
            vector<QuadFragments>& rasterized_points);

        static int uploadTexture2D(JTexture2D::ptr tex);
        static JTexture2D::ptr getTexture2D(int index);
        static int addLight(JLight::ptr lightSource);
        static JLight::ptr getLight(int index);
        static void setExposure(const float& _exposure) { exposure = _exposure; }
        static void setViewerPos(const glm::vec3& viewer) { viewerPos = viewer; }

        static glm::vec4 texture2D(const uint& id, const glm::vec2& uv, const glm::vec2& dUVdx, const glm::vec2& dUVdy);

    protected:
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat3 inveTransModelMatrix = glm::mat3(1.0f);
        glm::mat4 viewProjectMatrix = glm::mat4(1.0f);

        static vector<JTexture2D::ptr> globalTextureUnits;
        static vector<JLight::ptr> lights;
        static glm::vec3 viewerPos;
        static float exposure;

        glm::vec3 kA = glm::vec3(0.0f);
        glm::vec3 kD = glm::vec3(1.0f);
        glm::vec3 kS = glm::vec3(0.0f);
        glm::vec3 kE = glm::vec3(0.0f);
        float transparency = 1.0f;
        float shininess = 0.0f;
        int diffuseTexId = -1;
        int specularTexId = -1;
        int normalTexId = -1;
        int glowTexId = -1;
        bool lightingEnable = true;
    };
}

#endif //JSHADINGPIPELINE_H
