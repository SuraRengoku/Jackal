//
// Created by jonas on 2024/9/11.
//

#ifndef JRENDERER_H
#define JRENDERER_H

#include "glm/glm.hpp"
#include "SDL2/SDL.h"

#include "JFrameBuffer.h"
#include "JDrawableMesh.h"
#include "JShadingPipeline.h"
#include "JShadingState.h"

using std::vector;
using std::shared_ptr;
using std::string;
using std::array;
using uchar = unsigned char;

namespace JackalRenderer {
    class JRenderer final {
    public:
        using ptr = shared_ptr<JRenderer>;

        JRenderer(int width, int height);
        ~JRenderer() = default;

        void addDrawableMesh(JDrawableMesh::ptr mesh);
        void addDrawableMesh(const vector<JDrawableMesh::ptr>& meshes);
        void unloadDrawableMesh();

        void clearColor(const glm::vec4& color) const { backBuffer -> clearColor(color); }
        void clearDepth(const float& depth) const { backBuffer -> clearDepth(depth); }
        void clearColorAndDepth(const glm::vec4& color, const float& depth) const { backBuffer -> clearColorAndDepth(color, depth); }

        void setViewMatrix(const glm::mat4& view) { view_Matrix = view; }
        void setModeMatrix(const glm::mat4& model) { model_Matrix = model; }
        void setProjectMatrix(const glm::mat4& project, const float& near, const float& far) {
            project_Matrix = project;
            frustumNearFar = glm::vec2(near, far);
        }
        void setShaderPipeline(const JShadingPipeline::ptr& shader) { shaderHandler = shader; }
        void setViewerPos(const glm::vec3 &viewer);

        int addLightSource(JLight::ptr lightSource);
        JLight::ptr getLightSource(const int& idx);
        void setExposure(const float& exposure);

        uint renderAllDrawableMeshes();

        uint renderAllDrawableMesh(const size_t& idx);

        uchar* commitRenderedColorBuffer();

        static vector<JShadingPipeline::VertexData> clipingSutherlandHodgeman(
            const JShadingPipeline::VertexData& v0,
            const JShadingPipeline::VertexData& v1,
            const JShadingPipeline::VertexData& v2,
            const float& near,
            const float& far);

    private:
        static vector<JShadingPipeline::VertexData> clipingSutherlandHodgemanAux(
            const vector<JShadingPipeline::VertexData>& polygon,
            const int& axis,
            const int& side);

    private:
        vector<JDrawableMesh::ptr> drawable_meshes_;
        glm::mat4 model_Matrix = glm::mat4(1.0f); //local space -> world space
        glm::mat4 view_Matrix = glm::mat4(1.0f); //world space -> camera space
        glm::mat4 project_Matrix = glm::mat4(1.0f); //camera space -> clip space
        glm::mat4 viewport_Matrix = glm::mat4(1.0f); //ndc space -> screen space

        JShadingState shading_state_;

        glm::vec2 frustumNearFar;

        JShadingPipeline::ptr shaderHandler = nullptr;

        JFrameBuffer::ptr backBuffer;
        JFrameBuffer::ptr frontBuffer;
        vector<uchar> renderedImg; // 1 byte
    };
}


#endif //JRENDERER_H
