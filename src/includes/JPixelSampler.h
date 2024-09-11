//
// Created by jonas on 2024/6/24.
//

#ifndef JPIXELSAMPLER_H
#define JPIXELSAMPLER_H

#include <array>
#include <vector>
#include "glm/glm.hpp"

namespace JackalRenderer {
    template<typename T, size_t N>
    class JTPixelSampler {
    public:
        std::array<T, N> samplers; //type, number
        static size_t getSamplingNum() { return N; }
        T& operator[](const int &index) { return samplers[index]; }
        const T& operator[](const int &index) const { return samplers[index]; }
    };

    template<typename T>
    class J1PixelSampler : public JTPixelSampler<T, 1> {
    public:
        //assign each element in array with value
        J1PixelSampler(const T &value) { this->samplers.fill(value); }
        static const std::array<glm::vec2, 1> &getSamplingOffsets() {
            return { glm::vec2(0.0f, 0.0f) }; //分两个三角形区域，左上和右下
        }
    };

    template<typename T>
    class J2PixelSampler : public JTPixelSampler<T, 2> {
    public:
        J2PixelSampler(const T &value) { this->samplers.fill(value); }
        static const std::array<glm::vec2, 2> &getSamplingOffsets() {
            return { glm::vec2(-0.25f, -0.25f), glm::vec2(+0.25f, +0.25f) };
        }
    };

    template<typename T>
    class J4PixelSampler : public JTPixelSampler<T, 4> {
    public: //make sure this constructor is accessible
        J4PixelSampler(const T &value) { this->samplers.fill(value); }
        static const std::array<glm::vec2, 4> &getSamplingOffsets() {
            return {
                //RGSS
                //Refs: https://mynameismjp.wordpress.com/2012/10/24/msaa-overview/
                glm::vec2(+0.125f, +0.375f),
                glm::vec2(+0.375f, -0.125f),
                glm::vec2(-0.125f, -0.375f),
                glm::vec2(-0.375f, +0.125f)
            };
            // return {
            //     glm::vec2(+0.25f, +0.25f),
            //     glm::vec2(+0.25f, -0.25f),
            //     glm::vec2(-0.25f, -0.25f),
            //     glm::vec2(-0.25f, +0.25f)
            // };
        }
    };

    template<typename T>
    class J8PixelSampler : public JTPixelSampler<T, 8> {
    public:
        J8PixelSampler(const T &value) { this->samplers.fill(value); }
        static const std::array<glm::vec2, 8> &getSamplingOffsets() {
            return{
                //rooks
                glm::vec2(+0.0625f, -0.4375f),glm::vec2(+0.3125f, -0.0625f),
                glm::vec2(+0.4375f, +0.1875f),glm::vec2(+0.1875f,+0.3125f),
                glm::vec2(-0.0625f, +0.4375f),glm::vec2(-0.3125f, +0.0625f),
                glm::vec2(-0.4375f, -0.1875f),glm::vec2(-0.1875f,-0.3125f)
            };
        }
    };

#define MSAA4X

#ifdef MSAA4X
    template<typename T>
    using JPixelSampler = J4PixelSampler<T>;
#elif MSAA8X
    template<typename T>
    using JPixelSampler = J8PixelSampler<T>;
#elif MSAA2X
    template<typename T>
    using JPixelSampler = J2PixelSampler<T>;
#else
    template<typename T>
    using JPixelSampler = J1PixelSampler<T>;
#endif
    using JPixelRGB = std::array<unsigned char, 3>; //1 byte -> max: 255
    using JPixelRGBA = std::array<unsigned char, 4>;
    using JMaskPixelSampler = JPixelSampler<unsigned char>;
    using JDepthPixelSampler = JPixelSampler<float>;
    using JColorPixelSampler = JPixelSampler<JPixelRGBA>;
    //framebuffer attachment
    using JMaskBuffer = std::vector<JMaskPixelSampler>;
    using JDepthBuffer = std::vector<JDepthPixelSampler>;
    using JColorBuffer = std::vector<JColorPixelSampler>;

    constexpr JPixelRGBA jWhite = { 255, 255, 255, 255 };
    constexpr JPixelRGBA jBlack = { 0, 0, 0, 0};
}

#endif //JPIXELSAMPLER_H
