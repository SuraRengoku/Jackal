//
// Created by jonas on 2024/9/10.
//

#ifndef JDRAWABLEMESH_H
#define JDRAWABLEMESH_H


#include <vector>
#include <memory>
#include <string>
#include "glm/glm.hpp"
#include "JShadingState.h"

using std::vector;
using std::string;
using uint = unsigned int;

namespace JackalRenderer {
    class JVertex final {
    public:
        glm::vec3 vpostions = glm::vec3(0, 0, 0);
        glm::vec2 vtexcoords = glm::vec2(0, 0);
        glm::vec3 vnormals = glm::vec3(0, 1, 0);
        glm::vec3 vtangent;
        glm::vec3 vbitanget;
    };
    using JVertexBuffer = vector<JVertex>;
    using JIndexBuffer = vector<uint>;

    class JDrawableSubMesh {
    public:
        using ptr = std::shared_ptr<JDrawableSubMesh>;
        JDrawableSubMesh() = default;
        ~JDrawableSubMesh() = default;

        JDrawableSubMesh(const JDrawableSubMesh& mesh);
        JDrawableSubMesh& operator=(const JDrawableSubMesh& mesh);

        void setVertices(const vector<JVertex>& vertices) { this -> vertices = vertices; }
        void setIndices(const vector<uint>& indices) { this -> indices = indices; }

        void setDiffuseMapTexId(const int& id) { drawingMaterial.diffuseMapTexId = id; }
        void setSpecularMapTexId(const int& id) { drawingMaterial.specularMapTexId = id; }
        void setNormalMapTexId(const int& id) { drawingMaterial.normalMapTexId = id; }
        void setGlowMapTexId(const int& id) { drawingMaterial.glowMapTexId = id; }

        const int& getDiffuseMapTexId() const { return this -> drawingMaterial.diffuseMapTexId; }
        const int& getSpecularMapTexId() const { return this -> drawingMaterial.specularMapTexId; }
        const int& getNormalMapTexId() const { return this -> drawingMaterial.normalMapTexId; }
        const int& getGlowMapTexId() const { return this -> drawingMaterial.glowMapTexId; }

        JVertexBuffer& getVertices() { return this -> vertices; }
        JIndexBuffer& getIndices() { return this -> indices; }
        const vector<JVertex>& getVertices() const { return this -> vertices; }
        const vector<uint>& getIndices() const { return this -> indices; }

        void clear();
    protected:
        JVertexBuffer vertices;
        JIndexBuffer indices;
        struct DrawableMaterialTex {
            int diffuseMapTexId = -1;
            int specularMapTexId = -1;
            int normalMapTexId = -1;
            int glowMapTexId = -1;
        };
        DrawableMaterialTex drawingMaterial;
    };

    using JDrawableBuffer = vector<JDrawableSubMesh>;

    class JDrawableMesh {
    public:
        using ptr = std::shared_ptr<JDrawableMesh>;

        JDrawableMesh(const string& path, bool generatedMipmap);

        void setCullFaceMode(JCullFaceMode mode) { drawable_config.cullfacemode = mode; }
        void setDepthTestMode(JDepthTestMode mode) { drawable_config.depthtestmode = mode; }
        void setDepthWriteMode(JDepthWriteMode mode) { drawable_config.depthwritemode = mode; }
        void setAlphaBlendMode(JAlphaBlendingMode mode) { drawable_config.alphablendingmode = mode; }
        void setModeMatrix(const glm::mat4& mat) { drawable_config.modelMatrix = mat; }
        void setLightingMode(JLightingMode mode) { drawable_config.lighringmode = mode; }

        void setAmbientCoff(const glm::vec3& cof) { drawable_material_config.kA = cof; }
        void setDiffuseCoff(const glm::vec3& cof) { drawable_material_config.kD = cof; }
        void setSpeculerCoff(const glm::vec3& cof) { drawable_material_config.kS = cof; }
        void setEmissionCoff(const glm::vec3& cof) { drawable_material_config.kE = cof; }
        void setSpecularExponent(const float& cof) { drawable_material_config.shininess = cof; }
        void setTransparency(const float& alpha) { drawable_material_config.transparency = alpha; }

        JCullFaceMode getCullFaceMode() const { return drawable_config.cullfacemode; }
        JDepthTestMode getDepthTestMode() const { return drawable_config.depthtestmode; }
        JDepthWriteMode getDepthWriteMode() const { return drawable_config.depthwritemode; }
        JAlphaBlendingMode getAlphaBlendingMode() const { return drawable_config.alphablendingmode; }
        const glm::mat4& getModelMatrix() const { return drawable_config.modelMatrix; }
        JLightingMode getLightingMode() const { return drawable_config.lighringmode; }

        //函数返回const& 可以避免拷贝开销，也可以防止修改
        const glm::vec3& getAmbientCoff() const { return drawable_material_config.kA; }
        const glm::vec3& getDiffuseCoff() const { return drawable_material_config.kD; }
        const glm::vec3& getSpecularCoff() const { return drawable_material_config.kS; }
        const glm::vec3& getEmissionCoff() const { return drawable_material_config.kE; }
        const float& getSpecularExponent() const { return drawable_material_config.shininess; }
        const float& getTransparency() const { return drawable_material_config.transparency; }

        uint getDrawableMaxFaceNums() const;
        JDrawableBuffer& getDrawableSubMeshes() { return drawables; }

        void clear();

    protected:
        void importMeshFromFile(const string& path, bool generateMipmap = true);

        JDrawableBuffer drawables;
        struct DrawableConfig {
            JCullFaceMode cullfacemode = JCullFaceMode::J_CULL_BACK;
            JDepthTestMode depthtestmode = JDepthTestMode::J_DEPTH_TEST_ENABLE;
            JDepthWriteMode depthwritemode = JDepthWriteMode::J_DEPTH_WRITE_ENABLE;
            JAlphaBlendingMode alphablendingmode = JAlphaBlendingMode::J_ALPHA_DISABLE;
            JLightingMode lighringmode = JLightingMode::J_LIGHTING_ENABLE;
            glm::mat4 modelMatrix = glm::mat4(1.0f);
        };
        DrawableConfig drawable_config;

        struct DrawableMaterialConfig {
            glm::vec3 kA = glm::vec3(0.0f); //Ambient
            glm::vec3 kD = glm::vec3(1.0f); //Diffuse
            glm::vec3 kS = glm::vec3(0.0f); //Specular
            glm::vec3 kE = glm::vec3(0.0f); //Emission
            float shininess = 1.0f; // specular highlight exponent
            float transparency = 1.0f;
        };
        DrawableMaterialConfig drawable_material_config;
    };
}

#endif //JDRAWABLEMESH_H
