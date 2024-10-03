//
// Created by jonas on 2024/9/11.
//

#ifndef JSHADERPROGRAM_H
#define JSHADERPROGRAM_H

#include "JShadingPipeline.h"

using std::shared_ptr;
namespace JackalRenderer {
    class J3DShadingPipeline : public JShadingPipeline {
    public:
        using ptr = shared_ptr<J3DShadingPipeline>;

        virtual ~J3DShadingPipeline() = default;
        virtual void vertexShader(VertexData& vertex) const override;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JDoNothingShadingPipeline : public JShadingPipeline {
    public:
        using ptr = shared_ptr<JDoNothingShadingPipeline>;

        virtual ~JDoNothingShadingPipeline() = default;

        virtual void vertexShader(VertexData &vertex) const override;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JTextureShadingPipeline final : public J3DShadingPipeline {
    public:
        using ptr = shared_ptr<JTextureShadingPipeline>;

        virtual ~JTextureShadingPipeline() = default;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JLODVisualizePipeline final : public J3DShadingPipeline {
    public:
        using ptr = shared_ptr<JLODVisualizePipeline>;
        virtual ~JLODVisualizePipeline() = default;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JPhongShadingPipeling final : public J3DShadingPipeline {
    public:
        using ptr = shared_ptr<JPhongShadingPipeling>;
        virtual ~JPhongShadingPipeling() = default;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JBlinnPhongShadingPipeline final : public J3DShadingPipeline {
    public:
        using ptr = shared_ptr<JBlinnPhongShadingPipeline>;
        virtual  ~JBlinnPhongShadingPipeline() = default;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JBlinnPhongNormalMapShadingPipeline final : public J3DShadingPipeline {
    public:
        using ptr = shared_ptr<JBlinnPhongNormalMapShadingPipeline>;
        virtual ~JBlinnPhongNormalMapShadingPipeline() = default;
        virtual void vertexShader(VertexData &vertex) const override;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };

    class JAlphaBlendingShadingPipeline final : public J3DShadingPipeline {
    public:
        using ptr = shared_ptr<JAlphaBlendingShadingPipeline>;
        virtual ~JAlphaBlendingShadingPipeline() = default;
        virtual void fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const override;
    };
}

#endif //JSHADERPROGRAM_H
