//
// Created by jonas on 2024/6/25.
//

#ifndef JTEXTURE2D_H
#define JTEXTURE2D_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>

#include "glm/glm.hpp"

#include "JShadingState.h"
#include "JTextureContainer.h"

namespace JackalRenderer {
    class JTexture2D final {
    public:
        using ptr = std::shared_ptr<JTexture2D>;
        JTexture2D();
        JTexture2D(bool enablemipmap);
        ~JTexture2D() = default;

        bool isGeneratedMipmap() const { return enableMipmap; }
        int getWidth() const { return textureContainers[0] -> getWidth(); }
        int getHeight() const { return textureContainers[0] -> getHeight(); }

        void setWarpMode(JTextureWarpMode mode) { warpMode = mode; }
        void setFilterMode(JTextureFilterMode mode) { filterMode = mode; }

        bool loadTextureFromFile(
            const std::string &filepath,
            JTextureWarpMode warpmode = JTextureWarpMode::J_REPEAT,
            JTextureFilterMode filtermode = JTextureFilterMode::J_LINEAR);

        glm::vec4 sample(const glm::vec2 &uv, const float &level = 0.0f) const;

    private:
        void readPixel(const std::uint16_t &u, const std::uint16_t &v,
            uchar &r, uchar &g, uchar &b, uchar &a, const int level = 0) const;
        void generateMipmap(uchar *pixels, int width, int height, int channel);
        bool enableMipmap = false;
        std::vector<JTextureContainer::ptr> textureContainers;
        JTextureWarpMode warpMode;
        JTextureFilterMode filterMode;

        friend class JTexture2DSampler;
    };

    class JTexture2DSampler final {
    public:
        static glm::vec4 textureSamplingNearest(JTextureContainer::ptr texture, glm::vec2 uv);
        static glm::vec4 textureSamplingBilinear(JTextureContainer::ptr texture, glm::vec2 uv);
    };
}

#endif //JTEXTURE2D_H
