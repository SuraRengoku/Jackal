//
// Created by jonas on 2024/6/25.
//

#ifndef JSHADINGSTATE_H
#define JSHADINGSTATE_H

#include "glm/glm.hpp"

namespace JackalRenderer {
    enum JTextureWarpMode { J_REPEAT, J_MIRRORED_REPEAT, J_CLAMP_TO_EDGE };
    enum JTextureFilterMode { J_NEAREST, J_LINEAR };
    enum JCullFaceMode { J_CULL_DISABLE, J_CULL_FRONT, J_CULL_BACK };
    enum JDepthTestMode { J_DEPTH_TEST_DISABLE, J_DEPTH_TEST_ENABLE };
    enum JDepthWriteMode { J_DEPTH_WRITE_DISABLE, J_DEPTH_WRITE_ENABLE };
    enum JLightingMode { J_LIGHTING_DISABLE, J_LIGHTING_ENABLE };
    enum JAlphaBlendingMode { J_ALPHA_DISABLE, J_ALPHA_BLENDING, J_ALPHA_TO_COVERAGE };
    class JShadeingState {
        JCullFaceMode cullFaceMode = JCullFaceMode::J_CULL_BACK;
        JDepthTestMode depthTestMode = JDepthTestMode::J_DEPTH_TEST_ENABLE;
        JDepthWriteMode depthWriteMode = JDepthWriteMode::J_DEPTH_WRITE_ENABLE;
        JAlphaBlendingMode alphaBlendingMode = JAlphaBlendingMode::J_ALPHA_DISABLE;
    };
}

#endif //JSHADINGSTATE_H
