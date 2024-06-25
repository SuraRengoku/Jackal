//
// Created by jonas on 2024/6/24.
//

#ifndef JLIGHT_H
#define JLIGHT_H

#include <memory>
#include "glm/glm.hpp"

namespace JackalRenderer{
    class JLight{
    public:
        typedef std::shared_ptr<JLight> ptr;
        JLight() : Jintensity(glm::vec3(1.0f)) {}
        JLight(const glm::vec3 &intensity) :Jintensity(intensity) {}
        virtual ~JLight() = default;

        const glm::vec3 &intensity() const {return Jintensity;}
        virtual float attenuation(const glm::vec3 &fragPos) const = 0;
        virtual float cutoff(const glm::vec3 &lightDir) const = 0;
        virtual glm::vec3 direction(const glm::vec3 &fragPos) const = 0;

    protected:
        glm::vec3 Jintensity;
    };

    class JPointLight : public JLight {
    public:
        typedef std::shared_ptr<JPointLight> ptr;

        JPointLight(const glm::vec3 &intensity, const glm::vec3 &lightPos, const glm::vec3 &atten)
            : JLight(intensity), JlightPos(lightPos), JAttenuation(atten) {}

        virtual float attenuation(const glm::vec3 &fragPos) const override {
            float distance = glm::length(JlightPos - fragPos);
            return 1.0f / (JAttenuation.x + JAttenuation.y * distance + JAttenuation.z * (distance * distance));
        }
        virtual glm::vec3 direction(const glm::vec3 &fragPos) const override {
            return glm::normalize(JlightPos - fragPos);
        }
        virtual float cutoff(const glm::vec3 &lightDir) const override {
            return 1.0f;
        }
        glm::vec3 &getLightPos() {
            return JlightPos;
        }

    protected:
        glm::vec3 JlightPos;
        glm::vec3 JAttenuation;
    };
    // final, can be no longer inherited
    class JSpotLight final : public JPointLight {
    public:
        typedef std::shared_ptr<JSpotLight> ptr;

        JSpotLight(const glm::vec3 &intensity, const glm::vec3 &lightPos, const glm::vec3 &atten, const glm::vec3 &dir,
            const float &innerCutoff, const float &outerCutoff)
                : JPointLight(intensity, lightPos, atten), JSpotDir(glm::normalize(dir)), JinnerCutoff(innerCutoff), JouterCutoff(outerCutoff) {}
        virtual float cutoff(const glm::vec3 &lightDir) const override {
            float theta = glm::dot(lightDir, JSpotDir);
            static const float epsilon = JinnerCutoff - JouterCutoff;
            return glm::clamp((theta - JouterCutoff) / epsilon, 0.0f, 1.0f);
        }
        glm::vec3 &getSpotDir() {
            return JSpotDir;
        }
    protected:
        glm::vec3 JSpotDir;
        float JinnerCutoff;
        float JouterCutoff;
    };

    class JDirectLight final : public JLight {
    public:
        typedef std::shared_ptr<JDirectLight> ptr;
        JDirectLight(const glm::vec3 &intensity, const glm::vec3 &dir)
            : JLight(intensity), JLightDir(dir) {}
        virtual float attenuation(const glm::vec3 &fragPos) const override { return 1.0f; }
        virtual glm::vec3 direction(const glm::vec3 &fragPos) const override { return JLightDir; }
        virtual float cutoff(const glm::vec3 &lightDir) const override { return 1.0f; };
    protected:
        glm::vec3 JLightDir;
    };

}

#endif //JLIGHT_H
