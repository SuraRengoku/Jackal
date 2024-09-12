//
// Created by jonas on 2024/9/11.
//

#include "JShaderProgram.h"

namespace JackalRenderer {
    void J3DShadingPipeline::vertexShader(VertexData &vertex) const {
        vertex.pos = glm::vec3(modelMatrix * glm::vec4(vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0f));
        vertex.nor = glm::normalize(inveTransModelMatrix * vertex.nor);
        vertex.cpos = viewProjectMatrix * glm::vec4(vertex.pos, 1.0f);
    }

    void J3DShadingPipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(data.tex, 0.0f, 1.0f);
    }

    void JDoNothingShadingPipeline::vertexShader(VertexData &vertex) const {
        vertex.cpos = glm::vec4(vertex.pos, 1.0f);
    }

    void JDoNothingShadingPipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(data.tex, 0.0f, 1.0f);
    }

    void JTextureShadingPipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(kE, 1.0f);
        if(diffuseTexId != -1)
            fragColor = texture2D(diffuseTexId, data.tex, dUVdx, dUVdy);
    }

    void JLODVisualizePipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        static const glm::vec3 mipmapColors[] = {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(1.0f, 0.5f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.5f),
            glm::vec3(0.0f, 0.5f, 0.5f),
            glm::vec3(0.0f, 0.25f, 0.5f),
            glm::vec3(0.25f, 0.5f, 0.0f),
            glm::vec3(0.5f, 0.0f, 1.0f),
            glm::vec3(1.0f, 0.25f, 0.5f),
            glm::vec3(0.5f, 0.5f, 0.5f),
            glm::vec3(0.25f, 0.25f, 0.25f),
            glm::vec3(0.125f, 0.125f, 0.125f)
        };
        auto tex = JShadingPipeline::getTexture2D(diffuseTexId);
        int w = 1000, h = 100;
        if(tex != nullptr) {
            w = tex -> getWidth();
            h = tex -> getHeight();
        }
        glm::vec2 dfdx = dUVdx * glm::vec2(w, h);
        glm::vec2 dfdy = dUVdy * glm::vec2(w, h);
        float L = glm::max(glm::dot(dfdx, dfdx), glm::dot(dfdy, dfdy));
        float LOD = 0.5f * glm::log2(L);
        fragColor = glm::vec4(mipmapColors[glm::max(int(LOD + 0.5), 0)], 1.0f);
        return;
    }

    void JPhongShadingPipeling::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(0.0f); //没有光照时全黑, 如果想要鬼畜颜色也可以
        glm::vec3 Ambient, Diffuse, Specular, Emission;
        //textureContainer不对纹理类型进行区分，唯一可靠的是纹理编号
        glm::vec4 diffTexColor = (diffuseTexId != -1) ? texture2D(diffuseTexId, data.tex, dUVdx, dUVdy) : glm::vec4(1.0f);
        Ambient = Diffuse = (diffuseTexId != -1) ? glm::vec3(diffTexColor) : kD;
        Specular = (specularTexId != -1) ? glm::vec3(texture2D(specularTexId, data.tex, dUVdx, dUVdy)) : kS;
        Emission = (glowTexId != -1) ? glm::vec3(texture2D(glowTexId, data.tex, dUVdx, dUVdy)) : kE;

        if(! lightingEnable) {
            fragColor = glm::vec4(Emission, 1.0f);
            return;
        }

        //Phong 光照模型
        glm::vec3 fragPos = glm::vec3(data.pos);
        glm::vec3 normal = glm::vec3(data.nor);
        glm::vec3 viewDir = glm::normalize(viewerPos - fragPos);
#pragma unroll
        for(size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            glm::vec3 lightDir = light -> direction(fragPos);
            glm::vec3 ambient, diffuse, specular;
            float attenuation = 1.0f;
            {
                ambient = light -> intensity() * Ambient;
                float diffCof = glm::max(glm::dot(normal, lightDir), 0.0f);
                diffuse = light -> intensity() * Diffuse * diffCof * kD;
                glm::vec3 reflectDir = glm::reflect(-lightDir, normal);
                float specCof = std::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), shininess);
                specular = light -> intensity() * specCof * Specular;
                attenuation = light -> attenuation(fragPos);
            }
            float cutoff = light -> cutoff(lightDir);
            fragColor += glm::vec4((ambient + diffuse + specular) * attenuation * cutoff, 0.0f);
        }
        //alpha 通道用于混色
        fragColor = glm::vec4(fragColor.x + Emission.x, fragColor.y + Emission.y, fragColor.z + Emission.z, diffTexColor.a * transparency);
#ifdef HDR
        //HDR高光矫正
        {
            glm::vec3 hdrColor(fragColor);
            fragColor = glm::vec4(glm::vec3(1.0f - glm::exp(-hdrColor * exposure)), fragColor.a);
        }
#endif
    }

    void JBlinnPhongShadingPipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(0.0f);
        glm::vec3 Ambient, Diffuse, Specular, Emission;
        glm::vec4 diffTexColor = (diffuseTexId != -1) ? texture2D(diffuseTexId, data.tex, dUVdx, dUVdy) : glm::vec4(1.0f);
        Ambient = Diffuse = (diffuseTexId != -1) ? glm::vec3(diffTexColor) : kD;
        Specular = (specularTexId != -1) ? glm::vec3(texture2D(specularTexId, data.tex, dUVdx, dUVdy)) : kS;
        Emission = (glowTexId != -1) ? glm::vec3(texture2D(glowTexId, data.tex, dUVdx, dUVdy)) : kE;

        if(!lightingEnable) {
            fragColor = glm::vec4(Emission, diffTexColor.a);
            return;
        }

        glm::vec3 fragPos = glm::vec3(data.pos);
        glm::vec3 normal = glm::normalize(data.nor);
        glm::vec3 viewDir = glm::normalize(viewerPos - fragPos);
#pragma unroll
        for(size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            glm::vec3 lightDir = light -> direction(fragPos);
            glm::vec3 ambient, diffuse, specular;
            float attenuation = 1.0f;
            {
                ambient = light -> intensity() * Ambient * kA;
                float diffCof = glm::max(glm::dot(normal, lightDir), 0.0f);
                diffuse = light -> intensity() * diffCof * Diffuse * kD;
                glm::vec3 halfWay = glm::normalize(viewDir + lightDir);
                float specCof = glm::pow(glm::max(glm::dot(halfWay, normal), 0.0f), shininess);
                specular = light -> intensity() * specCof * Specular;
                attenuation = light -> attenuation(fragPos);
            }
            float cutoff = light -> cutoff(lightDir);
            fragColor += glm::vec4((ambient + diffuse + specular) * attenuation * cutoff, 0.0f);
        }
        fragColor = glm::vec4(fragColor.x + Emission.x, fragColor.y + Emission.y, fragColor.z + Emission.z, diffTexColor.a * transparency);

#ifdef HDR
        {
            glm::vec3 hdrColor(fragColor);
            fragColor = glm::vec4(glm::vec3(1.0f - glm::exp(-hdrColor * exposure)), fragColor.a);
        }
#endif
    }

    void JBlinnPhongNormalMapShadingPipeline::vertexShader(VertexData& vertex) const {
        vertex.pos = glm::vec3(modelMatrix * glm::vec4(vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0f));
        vertex.nor = glm::normalize(inveTransModelMatrix * vertex.nor);
        vertex.cpos = viewProjectMatrix * glm::vec4(vertex.pos, 1.0f);
        glm::vec3 T = glm::normalize(inveTransModelMatrix * vertex.tbn[0]);
        glm::vec3 B = glm::normalize(inveTransModelMatrix * vertex.tbn[1]);
        vertex.tbn = glm::mat3(T, B, vertex.nor);
        vertex.needInterpolatedTBN = true;
    }

    void JBlinnPhongNormalMapShadingPipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(0.0f);
        glm::vec3 Ambient, Diffuse, Specular, Emission;
        glm::vec4 diffTexColor = (diffuseTexId != -1) ? texture2D(diffuseTexId, data.tex, dUVdx, dUVdy) : glm::vec4(1.0f);
        Ambient = Diffuse = (diffuseTexId != -1) ? glm::vec3(diffTexColor) : kD;
        Specular = (specularTexId != -1) ? glm::vec3(texture2D(specularTexId, data.tex, dUVdx, dUVdy)) : kS;
        Emission = (glowTexId != -1) ? glm::vec3(texture2D(glowTexId, data.tex, dUVdx, dUVdy)) : kE;

        if(!lightingEnable) {
            fragColor = glm::vec4(Emission, 1.0f);
            return;
        }

        glm::vec3 normal = data.nor;
        if(normalTexId != -1) {
            normal = glm::vec3(texture2D(normalTexId, data.tex, dUVdx, dUVdy)) * 2.0f - glm::vec3(1.0f);
            normal = data.tbn * normal;
        }
        normal = glm::normalize(normal);

        glm::vec3 fragPos = glm::vec3(data.pos);
        glm::vec3 viewDir = glm::normalize(viewerPos - fragPos);
#pragma unroll
        for(size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            glm::vec3 lightDir = light -> direction(fragPos);

            glm::vec3 ambient, diffuse, specular;
            float attenuation = 1.0f;
            {
                ambient = light -> intensity() * Ambient;
                float diffCof = glm::max(glm::dot(normal, lightDir), 0.0f);
                diffuse = light -> intensity() * diffCof * Diffuse * kD;
                glm::vec3 halfWay = glm::normalize(viewDir + lightDir);
                float specCof = glm::pow(glm::max(glm::dot(halfWay, normal), 0.0f), shininess);
                specular = light -> intensity() * specCof * Specular;

                attenuation = light -> attenuation(fragPos);
            }
            float cutoff = light -> cutoff(lightDir);
            fragColor += glm::vec4((ambient + diffuse + specular) * attenuation * cutoff, 0.0f);
        }
        fragColor = glm::vec4(fragColor.x + Emission.x, fragColor.y + Emission.y, fragColor.z + Emission.z, diffTexColor.a * transparency);
#ifdef HDR
        {
            glm::vec3 hdrColor(fragColor);
            fragColor = glm::vec4(glm::vec3(1.0f - glm::exp(-hdrColor * exposure)), fragColor.a);
        }
#endif
    }

    void JAlphaBlendingShadingPipeline::fragmentShader(const FragmentData &data, glm::vec4 &fragColor, const glm::vec2 &dUVdx, const glm::vec2 &dUVdy) const {
        fragColor = glm::vec4(kE, 1.0f);
        if(diffuseTexId != -1)
            fragColor = texture2D(diffuseTexId, data.tex, dUVdx, dUVdy);
        fragColor.a *= transparency;
    }
}