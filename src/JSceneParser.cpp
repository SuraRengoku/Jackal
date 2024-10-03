//
// Created by jonas on 2024/9/27.
//
#include "JSceneParser.h"

#include "JParallelWrapper.h"

namespace JackalRenderer {
    JDrawableMesh::ptr JSceneParser::getObject(const string &name) {
        if(scene.objects.find(name) == scene.objects.end())
            return nullptr;
        return scene.objects[name];
    }

    int JSceneParser::getLight(const string& name) {
        if(scene.lights.find(name) == scene.lights.end())
            return -1;
        return scene.lights[name];
    }

    void JSceneParser::parse(const string& path, JRenderer::ptr renderer, bool generatedMipmap) {
        std::ifstream sceneFile;
        sceneFile.open(path, std::ios::in);
        if(!sceneFile.is_open()) {
            std::cerr << "File does not exist: " << path << std::endl;
            exit(1);
        }
        string line;
        while(std::getline(sceneFile, line)) {
            if (line.empty())
                continue;
            std::stringstream ss;
            ss << line;
            string header;
            ss >> header;
            if (header == "Config:") {
                std::cout << "Config: ============================================\n";
                float exposure = 1.0f;
                {
                    std::getline(sceneFile, line);
                    exposure = parseFloat(line);
                }
                renderer->setExposure(exposure);
            } else if (header == "Camera:") {
                std::cout << "Camera: ============================================\n";
                {
                    std::getline(sceneFile, line);
                    scene.cameraPos = parseVec3(line);
                }
                {
                    std::getline(sceneFile, line);
                    scene.cameraFocus = parseVec3(line);
                }
                {
                    std::getline(sceneFile, line);
                    scene.cameraUp = parseVec3(line);
                }
            } else if (header == "Frustum:") {
                std::cout << "Frustum: ===========================================\n";
                {
                    std::getline(sceneFile, line);
                    scene.frustumFovy = parseFloat(line);
                }
                {
                    std::getline(sceneFile, line);
                    scene.frustumNear = parseFloat(line);
                }
                {
                    std::getline(sceneFile, line);
                    scene.frustumFar = parseFloat(line);
                }
            } else if (header == "PointLight:") {
                std::cout << "PointLight:=====================================\n";
                std::string name;
                {
                    std::getline(sceneFile, line);
                    name = parseStr(line);
                }

                glm::vec3 pos;
                {
                    std::getline(sceneFile, line);
                    pos = parseVec3(line);
                }

                glm::vec3 atten;
                {
                    std::getline(sceneFile, line);
                    atten = parseVec3(line);
                }

                glm::vec3 color;
                {
                    std::getline(sceneFile, line);
                    color = parseVec3(line);
                }
                JLight::ptr lightSource = std::make_shared<JPointLight>(color, pos, atten);
                scene.lights[name] = renderer->addLightSource(lightSource);
            } else if (header == "SpotLight:") {
                std::cout << "SpotLight:=====================================\n";
                std::string name;
                {
                    std::getline(sceneFile, line);
                    name = parseStr(line);
                }

                glm::vec3 pos;
                {
                    std::getline(sceneFile, line);
                    pos = parseVec3(line);
                }

                glm::vec3 atten;
                {
                    std::getline(sceneFile, line);
                    atten = parseVec3(line);
                }

                glm::vec3 color;
                {
                    std::getline(sceneFile, line);
                    color = parseVec3(line);
                }

                float innerCutoff;
                {
                    std::getline(sceneFile, line);
                    innerCutoff = parseFloat(line);
                }

                float outerCutoff;
                {
                    std::getline(sceneFile, line);
                    outerCutoff = parseFloat(line);
                }

                glm::vec3 spotDir;
                {
                    std::getline(sceneFile, line);
                    spotDir = parseVec3(line);
                }

                JLight::ptr lightSource = std::make_shared<JSpotLight>(color, pos, atten, spotDir,
                    glm::cos(glm::radians(innerCutoff)), glm::cos(glm::radians(outerCutoff)));
                scene.lights[name] = renderer->addLightSource(lightSource);
            } else if (header == "DirectionalLight:") {
                std::cout << "DirectionalLight:=====================================\n";
                std::string name;
                {
                    std::getline(sceneFile, line);
                    name = parseStr(line);
                }

                glm::vec3 dir;
                {
                    std::getline(sceneFile, line);
                    dir = parseVec3(line);
                }

                glm::vec3 color;
                {
                    std::getline(sceneFile, line);
                    color = parseVec3(line);
                }

                JLight::ptr lightSource = std::make_shared<JDirectLight>(color, dir);
                scene.lights[name] = renderer->addLightSource(lightSource);
            } else if (header == "Object:") {
                std::cout << "Object: ==============================================\n";
                string name;
                {
                    std::getline(sceneFile, line);
                    name = parseStr(line);
                }
                std::getline(sceneFile, line);
                string path = parseStr(line);
                JDrawableMesh::ptr drawable = std::make_shared<JDrawableMesh>(path, generatedMipmap);
                renderer -> addDrawableMesh(drawable);
                scene.objects[name] = drawable;

                {
                    glm::vec3 translate;
                    {
                        std::getline(sceneFile, line);
                        translate = parseVec3(line);
                    }
                    glm::vec3 rotation;
                    {
                        std::getline(sceneFile, line);
                        rotation = parseVec3(line);
                    }
                    glm::vec3 scale;
                    {
                        std::getline(sceneFile, line);
                        scale = parseVec3(line);
                    }
                    glm::mat4 modelMatrix(1.0f);
                    modelMatrix = glm::translate(modelMatrix, translate);
                    modelMatrix = glm::scale(modelMatrix, scale);
                    drawable -> setModeMatrix(modelMatrix);
                }

                {
                    std::getline(sceneFile, line);
                    bool lighting = parseBool(line);
                    drawable -> setLightingMode(lighting ? JLightingMode::J_LIGHTING_ENABLE : JLightingMode::J_LIGHTING_DISABLE);
                }

                {
                    std::getline(sceneFile, line);
                    string cullface = parseStr(line);
                    if (cullface == "back")
                        drawable -> setCullFaceMode(JCullFaceMode::J_CULL_BACK);
                    else if (cullface == "front")
                        drawable -> setCullFaceMode(JCullFaceMode::J_CULL_FRONT);
                    else
                        drawable -> setCullFaceMode(JCullFaceMode::J_CULL_DISABLE);
                }

                {
                    std::getline(sceneFile, line);
                    bool depthtest = parseBool(line);
                    drawable -> setDepthTestMode(depthtest ? JDepthTestMode::J_DEPTH_TEST_ENABLE : J_DEPTH_TEST_DISABLE);
                }

                {
                    std::getline(sceneFile, line);
                    bool depthwrite = parseBool(line);
                    drawable -> setDepthWriteMode(depthwrite ? JDepthWriteMode::J_DEPTH_WRITE_ENABLE : J_DEPTH_WRITE_DISABLE);
                }

                {
                    std::getline(sceneFile, line);
                    string blend = parseStr(line);
                    if (blend == "alphablend")
                        drawable -> setAlphaBlendMode(JAlphaBlendingMode::J_ALPHA_BLENDING);
                    else if (blend == "alpha2coverage")
                        drawable -> setAlphaBlendMode(JAlphaBlendingMode::J_ALPHA_TO_COVERAGE);
                    else
                        drawable -> setAlphaBlendMode(JAlphaBlendingMode::J_ALPHA_DISABLE);
                }

                std::getline(sceneFile, line);
                {
                    {
                        float alpha;
                        std::getline(sceneFile, line);
                        alpha = parseFloat(line);
                        drawable -> setTransparency(alpha);
                    }
                    {
                        float ns;
                        std::getline(sceneFile, line);
                        ns = parseFloat(line);
                        drawable -> setSpecularExponent(ns);
                    }
                    {
                        glm::vec3 ka;
                        std::getline(sceneFile, line);
                        ka = parseVec3(line);
                        drawable -> setAmbientCoff(ka);
                    }
                    {
                        glm::vec3 kd;
                        std::getline(sceneFile, line);
                        kd = parseVec3(line);
                        drawable->setDiffuseCoff(kd);
                    }
                    {
                        glm::vec3 ks;
                        std::getline(sceneFile, line);
                        ks = parseVec3(line);
                        drawable->setSpecularCoff(ks);
                    }
                    {
                        glm::vec3 ke;
                        std::getline(sceneFile, line);
                        ke = parseVec3(line);
                        drawable->setEmissionCoff(ke);
                    }
                }
            }
        }
        sceneFile.close();
    }

    float JSceneParser::parseFloat(string str) const {
        std::stringstream ss;
        string token;
        ss << str;
        ss >> token;
        float ret;
        ss >> ret;
        std::cout << token << " " << ret << std::endl;
        return ret;
    }

    glm::vec3 JSceneParser::parseVec3(string str) const {
        std::stringstream ss;
        string token;
        ss << str;
        ss >> token;
        glm::vec3 ret;
        ss >> ret.x;
        ss >> ret.y;
        ss >> ret.z;
        std::cout << token << " " << ret.x << "," << ret.y << "," << ret.z << std::endl;
        return ret;
    }

    bool JSceneParser::parseBool(string str) const {
        std::stringstream ss;
        string token;
        ss << str;
        ss >> token;
        bool ret = true;
        string tag;
        ss >> tag;
        if (tag == "true")
            ret = true;
        if (tag == "false")
            ret = false;
        std::cout << token << " " << (ret ? "true" : "false") << std::endl;
        return ret;
        // TODO original
        // return true;
    }

    string JSceneParser::parseStr(string str) const {
        std::stringstream ss;
        string token;
        ss << str;
        ss >> token;
        string ret;
        ss >> ret;
        std::cout << token << " " << ret << std::endl;
        return ret;
    }
}
