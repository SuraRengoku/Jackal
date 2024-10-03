//
// Created by jonas on 2024/6/24.
//

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "JWindowsApp.h"
#include "JRenderer.h"
#include "JMathUtils.h"
#include "JShaderProgram.h"
#include "JSceneParser.h"

#include <iostream>

using namespace JackalRenderer;

int main(int argc, char* args[]) {
    constexpr int width = 700;
    constexpr int height = 500;

    JWindowsApp::ptr winApp = JWindowsApp::getInstance(width, height, "JackalRenderer");

    if (winApp == nullptr)
        return -1;

    bool generateMipmap = true;
    JRenderer::ptr renderer = std::make_shared<JRenderer>(width, height);

    JSceneParser parser;
    parser.parse("../../scenes/directionallight.scene", renderer, generateMipmap);

    renderer->setViewMatrix(JMathUtils::calcViewMatrix(parser.scene.cameraPos, parser.scene.cameraFocus, parser.scene.cameraUp));
    renderer->setProjectMatrix(JMathUtils::calsPersProjectMatrix(parser.scene.frustumFovy, static_cast<float>(width) / height, parser.scene.frustumNear, parser.scene.frustumFar), parser.scene.frustumNear, parser.scene.frustumFar);

    winApp->readytoStart();

    renderer->setShaderPipeline(std::make_shared<JBlinnPhongShadingPipeline>());

    glm::vec3 cameraPos = parser.scene.cameraPos;
    glm::vec3 lookAtTarget = parser.scene.cameraFocus;

    while (!winApp->shouldWindowClose()) {
        winApp->processEvent();
        renderer->clearColorAndDepth(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0f);
        renderer->setViewerPos(cameraPos);
        auto numTriangles = renderer->renderAllDrawableMeshes();
        double deltaTime = winApp->updateScreenSurface(renderer->commitRenderedColorBuffer(), width, height, 3, numTriangles);

        {
            if (winApp->getISMouseLeftButtonPressed()) {
                int deltaX = winApp->getMouseMotionDeltaX();
                int deltaY = winApp->getMouseMotionDeltaY();
                glm::mat4 cameraRotMat(1.0f);
                if (std::abs(deltaX) > std::abs(deltaY))
                    cameraRotMat = glm::rotate(glm::mat4(1.0f), -deltaX * 0.001f, glm::vec3(0, 1, 0));
                else
                    cameraRotMat = glm::rotate(glm::mat4(1.0f), -deltaY * 0.001f, glm::vec3(1, 0, 0));

                cameraPos = glm::vec3(cameraRotMat * glm::vec4(cameraPos, 1.0f));
                renderer->setViewMatrix(JMathUtils::calcViewMatrix(cameraPos, lookAtTarget, glm::vec3(0.0f, 1.0f, 0.0f)));
            }

            if (winApp->getMouseWheelDelta() != 0) {
                glm::vec3 dir = glm::normalize(cameraPos - lookAtTarget);
                float dist = glm::length(cameraPos - lookAtTarget);
                glm::vec3 newPos = cameraPos + (winApp->getMouseWheelDelta() * 0.1f) * dir;
                if (glm::length(newPos - lookAtTarget) > 1.0f) {
                    cameraPos = newPos;
                    renderer->setViewMatrix(JMathUtils::calcViewMatrix(cameraPos, lookAtTarget, glm::vec3(0.0f, 1.0f, 0.0f)));
                }
            }
        }
    }
    renderer->unloadDrawableMesh();
    return 0;
}