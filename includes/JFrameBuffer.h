//
// Created by jonas on 2024/6/24.
//

#ifndef JFRAMEBUFFER_H
#define JFRAMEBUFFER_H

#include <vector>
#include <memory>

#include "glm/glm.hpp"
#include "JPixelSampler.h"

namespace JackalRenderer {
    using uint = unsigned int;

    class JFrameBuffer final {
    public:
        using ptr = std::shared_ptr<JFrameBuffer>;

        JFrameBuffer(int width, int height);
        ~JFrameBuffer() = default;
        void clearDepth(const float &depth);
        void clearColor(const glm::vec4 &color);
        void clearColorAndDepth(const glm::vec4 &color, const float &depth);

        int getWidth() const { return width; }
        int getHeight() const { return height; }
        const JDepthBuffer &getDepthBuffer() const { return depthBuffer; }
        const JColorBuffer &getColorBuffer() const {return colorBuffer; }

        float readDepth(const uint &x, const uint &y, const uint &i) const;
        // using TRPixelRGBA = std::array<unsigned char, 4>;
        JPixelRGBA readColor(const uint &x, const uint &y, const uint &i) const;

        void writeDepth(const uint &x, const uint &y, const uint &i, const float &value);
        void writeColor(const uint &x, const uint &y, const uint &i, const glm::vec4 &color);
        // using JMaskPixelSampler = JPixelSampler<unsigned char>;
        void writeColorWithMask(const uint &x, const uint &y, const glm::vec4 &color, const JMaskPixelSampler &mask);
        void writeColorWithMaskAlphaBlending(const uint &x, const uint &y, const glm::vec4 &color, const JMaskPixelSampler &mask);
        void writeDepthWithMask(const uint &x, const uint &y, const JDepthPixelSampler &depth, const JMaskPixelSampler &mask);

        // using JColorBuffer = std::vector<JColorPixelSampler>;
        const JColorBuffer &resolve();
    private:
        JDepthBuffer depthBuffer;
        JColorBuffer colorBuffer;
        unsigned int width, height;
    };

}
#endif //JFRAMEBUFFER_H
