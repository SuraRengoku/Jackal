//
// Created by jonas on 2024/9/27.
//

#ifndef JSCENEPARSER_H
#define JSCENEPARSER_H

#include <map>
#include <string>
using std::map;
using std::string;
#include <fstream>
#include <sstream>
#include <iostream>

#include "glm/gtc/matrix_transform.hpp"
#include "JLight.h"

#include "JRenderer.h"
#include "JDrawableMesh.h"

namespace JackalRenderer {
    class JSceneParser final {
    public:
        struct  Config{
            glm::vec3 cameraPos;
            glm::vec3 cameraFocus;
            glm::vec3 cameraUp;

            float frustumFovy;
            float frustumNear;
            float frustumFar;

            map<string, int> lights;
            map<string, JDrawableMesh::ptr> objects;
        } scene;

        JDrawableMesh::ptr getObject(const string& name);
        int getLight(const string& name);
        void parse(const string& path, JRenderer::ptr renderer, bool generatedMipmap);
    private:
        float parseFloat(string str) const;
        glm::vec3 parseVec3(string str) const;
        bool parseBool(string str) const;
        string parseStr(string str) const;
    };
}

#endif //JSCENEPARSER_H
