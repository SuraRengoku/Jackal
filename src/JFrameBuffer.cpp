//
// Created by jonas on 2024/6/24.
//

#include <cmath>
#include <algorithm>

#include "JFrameBuffer.h"
#include "JParallelWrapper.h"

namespace JackalRenderer {
    using uchar = unsigned char;

    JFrameBuffer::JFrameBuffer(int width, int height) : width(width), height(height) {
        depthBuffer.resize(width * height, 1.0f); // rendering area
        colorBuffer.resize(width * height, jBlack);
    }

    float JFrameBuffer::readDepth(const uint &x, const uint &y, const uint &i) const {
        if(x >= width || y >= height) return 0.0f;
        return depthBuffer[y * width + x][i]; // i is the index of sampling point (MSAA4x, MSAA8x)
    }

    JPixelRGBA JFrameBuffer::readColor(const uint &x, const uint &y, const uint &i) const {
        if(x >= width || y >= height) return jWhite; // { 255, 255, 255, 255}
        return colorBuffer[y * width +x][i];
    }

    void JFrameBuffer::clearDepth(const float &depth) { //reset
        parallelLoop((size_t)0, (size_t)width * height, [&](const size_t &ind){ depthBuffer[ind] = depth; });
    }

    void JFrameBuffer::clearColor(const glm::vec4 &color) { // value range (0, 1) step 1/255
        uchar red = static_cast<uchar>(255 * color.x);
        uchar green = static_cast<uchar>(255 * color.y);
        uchar blue = static_cast<uchar>(255 * color.z);
        uchar alpha = static_cast<uchar>(255 * color.w);
        JPixelRGBA rgba = { red, green, blue, alpha };
        parallelLoop((size_t)0, (size_t)width * height, [&](const size_t &ind){ colorBuffer[ind] = rgba; });
    }

    void JFrameBuffer::clearColorAndDepth(const glm::vec4 &color, const float &depth) {
        uchar red = static_cast<uchar>(255 * color.x);
        uchar green = static_cast<uchar>(255 * color.y);
        uchar blue = static_cast<uchar>(255 * color.z);
        uchar alpha = static_cast<uchar>(255 * color.w);
        JPixelRGBA rgba = { red, green, blue, alpha };
        parallelLoop((size_t)0, (size_t)width * height, [&](const size_t &ind) {
            colorBuffer[ind] = rgba; // for each sampling point(1 - 4 - 8), fill rgba
            depthBuffer[ind] = depth; // for each sampling point(1 - 4 - 8), fill depth
        });
    }

    void JFrameBuffer::writeColor(const uint &x, const uint &y, const uint &i, const glm::vec4 &color) {
        if(x>= width || y>= height) return;
        uchar red = static_cast<uchar>(255 * color.x);
        uchar green = static_cast<uchar>(255 * color.y);
        uchar blue = static_cast<uchar>(255 * color.z);
        uchar alpha = static_cast<uchar>(glm::min(255 * color.w, 255.0f)); // transparent: 0 opaque: 255
        JPixelRGBA rgba = { red, green, blue, alpha };
        colorBuffer[y * width + x][i] = rgba;
    }

    void JFrameBuffer::writeColorWithMask(const uint &x, const uint &y, const glm::vec4 &color, const JMaskPixelSampler &mask) {
        if(x>= width || y>= height) return;
        uchar red = static_cast<uchar>(255 * color.x);
        uchar green = static_cast<uchar>(255 * color.y);
        uchar blue = static_cast<uchar>(255 * color.z);
        uchar alpha = static_cast<uchar>(255 * color.w);
        JPixelRGBA rgba = { red, green, blue, alpha };

        int ind = y * width + x;
#pragma unroll // unfold the loop to accelerate, try to avoid computation in this part
        for(int i = 0; i < mask.getSamplingNum(); ++i) {
            //only write color if the corresponding mask equals to 1
            if(mask[i] == 1)
                colorBuffer[ind][i] =rgba; //write single sampling point
        }
    }

    void JFrameBuffer::writeColorWithMaskAlphaBlending(const uint &x, const uint &y, const glm::vec4 &color, const JMaskPixelSampler &mask) {
        if(x>= width || y>= height) return;
        uchar red = static_cast<uchar>(255 * color.x);
        uchar green = static_cast<uchar>(255 * color.y);
        uchar blue = static_cast<uchar>(255 * color.z);
        uchar alpha = static_cast<uchar>(255 * color.w);
        JPixelRGBA rgba = { red, green, blue, alpha };

        const float srcAlpha = color.a;
        const float desAlpha = 1.0f - srcAlpha;
        int ind = y * width + x;
        // Refs: https://learnopengl-cn.github.io/04%20Advanced%20OpenGL/03%20Blending/
        // full blending
#pragma unroll
        for(int i = 0; i < mask.getSamplingNum(); ++i) {
            if(mask[i] == 1) {
                colorBuffer[ind][i][0] = srcAlpha * rgba[0] + desAlpha * colorBuffer[ind][i][0];
                colorBuffer[ind][i][1] = srcAlpha * rgba[1] + desAlpha * colorBuffer[ind][i][1];
                colorBuffer[ind][i][2] = srcAlpha * rgba[2] + desAlpha * colorBuffer[ind][i][2];
                colorBuffer[ind][i][3] = srcAlpha * rgba[3] + desAlpha * colorBuffer[ind][i][3];
            }
        }
    }

    void JFrameBuffer::writeDepth(const uint &x, const uint &y, const uint &i, const float &value) {
        if(x>= width || y>= height) return;
        depthBuffer[y * width + x][i] = value;
    }

    void JFrameBuffer::writeDepthWithMask(const uint &x, const uint &y, const JDepthPixelSampler &depth, const JMaskPixelSampler &mask) {
        if(x>= width || y>= height) return;
        int ind = y * width + x;
#pragma unroll
        for(int i = 0; i < mask.getSamplingNum(); ++i) {
            if(mask[i] == 1)
                depthBuffer[ind][i] = depth[i];
        }
    }

    const JColorBuffer &JFrameBuffer::resolve() {
        parallelLoop((size_t)0, (size_t)width * height, [&](const size_t &index) {
            auto &currentSample = colorBuffer[index];
            glm::vec4 sum(0.0f);
            #pragma unroll
            for (int i = 0; i < currentSample.getSamplingNum(); ++i){
                    sum.x += currentSample[i][0];
                    sum.y += currentSample[i][1];
                    sum.z += currentSample[i][2];
                    sum.w += currentSample[i][3];
            }
            sum /= currentSample.getSamplingNum();
            JPixelRGBA rgba;
            rgba[0] = static_cast<unsigned char>(sum.x);
            rgba[1] = static_cast<unsigned char>(sum.y);
            rgba[2] = static_cast<unsigned char>(sum.z);
            rgba[3] = static_cast<unsigned char>(sum.w);
            currentSample[0] = rgba;
        }, JExecutionPolicy::J_PARALLEL);
        return colorBuffer;
    }
}

