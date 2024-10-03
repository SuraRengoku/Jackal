//
// Created by jonas on 2024/6/25.
//

#include "JTexture2D.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "JParallelWrapper.h"

namespace JackalRenderer {
    JTexture2D::JTexture2D() :
        enableMipmap(false),
        warpMode(JTextureWarpMode::J_MIRRORED_REPEAT),
        filterMode((JTextureFilterMode::J_LINEAR)) {}

    JTexture2D::JTexture2D(bool enablemipmap) :
         enableMipmap(enablemipmap),
         warpMode(JTextureWarpMode::J_MIRRORED_REPEAT),
         filterMode(JTextureFilterMode::J_LINEAR) {}

    bool JTexture2D::loadTextureFromFile(const std::string &filepath, JTextureWarpMode warpmode, JTextureFilterMode filtermode) {
        warpMode = warpmode;
        filterMode = filtermode;
        std::vector<JTextureContainer::ptr>().swap(textureContainers);
        // construct a new empty JTextureContainer pointer, swap the pointer of current content in textureContainers with the of pointer of empty JTextureContainer

        int width, height, nrComponents;
        try {
            uchar *data = stbi_load(filepath.c_str(), &width, &height, &nrComponents, 0);
            if(width <= 0 || width >= 65536 || height <= 0 || height >= 65536)
                throw std::runtime_error("Invalid image size from: " + filepath);
            if(data) {
                uchar *allPixels = new uchar[width * height * 4];
                parallelLoop((size_t)0, (size_t)(width * height), [&](const int &ind) {
                    int pos = ind * 4;
                    uchar &r = allPixels[pos + 0];
                    uchar &g = allPixels[pos + 1];
                    uchar &b = allPixels[pos + 2];
                    uchar &a = allPixels[pos + 3];
                    int posInData = ind * nrComponents;
                    switch(nrComponents) {
                        case 1:
                            r = g = b = allPixels[posInData], a = 255;
                            break;
                        case 3:
                            r = allPixels[posInData], g = allPixels[posInData + 1], b = allPixels[posInData + 2], a = 255;
                            break;
                        case 4:
                            r = allPixels[posInData], g = allPixels[posInData + 1], b = allPixels[posInData + 2], a = allPixels[posInData + 3];
                            break;
                        default:
                            r = g = b = allPixels[posInData], a = 255;
                            break;
                    }
                }, JExecutionPolicy::J_PARALLEL);
                nrComponents = 4;
                stbi_image_free(data);
                data = nullptr;
                if(enableMipmap)
                    generateMipmap(allPixels, width, height, nrComponents);
                else
                    textureContainers = { std::make_shared<JSwizzlingTextureContainer>(allPixels, width, height, nrComponents) };
                delete [] allPixels;
                return true;
            }
            else if(data == nullptr)
                throw std::runtime_error("Failed to load image from: " + filepath);
        } catch (const std::runtime_error& err) {
            std::cerr<< err.what() << std::endl;
            throw;
        }
    }

    void JTexture2D::generateMipmap(uchar *pixels, int width, int height, int channel) {
        uchar *rawData = pixels;
        bool reAlloc = false;

        static auto firstGreaterPowOf2 = [](const int &num) -> int {
            int base = num -1;
            base |= (base >> 1);
            base |= (base >> 2);
            base |= (base >> 4);
            base |= (base >> 8);
            base |= (base >> 16);
            return base + 1;
        };
        int nw = firstGreaterPowOf2(width);
        int nh = firstGreaterPowOf2(height);

        if(nw != width || nh != height || nw != nh) {
            nh = nw = glm::max(nw, nh);
            reAlloc = true;
            rawData = new uchar[nw * nh * channel];
            auto readPixels = [&](const int &x, const int &y, uchar &r, uchar &g, uchar &b, uchar &a) -> void {
                int tx = (x >= width) ? (width -1) : x;
                int ty = (y >= height) ? (height -1) : y;
                r = pixels[(ty + width +tx) * channel + 0];
                g = pixels[(ty + width +tx) * channel + 1];
                b = pixels[(ty + width +tx) * channel + 2];
                a = pixels[(ty + width +tx) * channel + 3];
            };

            parallelLoop((size_t)0, (size_t)(nw * nh), [&](const int &index) -> void {
                float x = (float)(index % nw) / (float)(nw - 1) * (width - 1); //formal part is rate
                float y = (float)(index % nw) / (float)(nh - 1) * (height - 1);
                uchar &r = rawData[index * channel];
                uchar &g = rawData[index * channel + 1];
                uchar &b = rawData[index * channel + 2];
                uchar &a = rawData[index * channel + 3];
                {
                    int xind = static_cast<int>(x), yind = static_cast<int>(y);
                    float fx = x - xind, fy = y - yind;
                    uchar quad[4][4];
                    readPixels(xind, yind, quad[0][0], quad[0][1], quad[0][2], quad[0][3]);
                    readPixels(xind + 1, yind, quad[1][0], quad[1][1], quad[1][2], quad[1][3]);
                    readPixels(xind, yind + 1, quad[2][0], quad[2][1], quad[2][2], quad[2][3]);
                    readPixels(xind + 1, yind + 1, quad[3][0], quad[3][1], quad[3][2], quad[3][3]);
                    float w0 = (1.0f - fx) * (1.0f - fy), w1 = fx * (1.0f - fy);
                    float w2 = (1.0f - fx) * fy, w3 = fx * fy;
                    r = uchar(w0 * quad[0][0] + w1 * quad[1][0] + w2 * quad[2][0] + w3 * quad[3][0]);
                    g = uchar(w0 * quad[0][1] + w1 * quad[1][1] + w2 * quad[2][1] + w3 * quad[3][1]);
                    b = uchar(w0 * quad[0][2] + w1 * quad[1][2] + w2 * quad[2][2] + w3 * quad[3][2]);
                    a = uchar(w0 * quad[0][3] + w1 * quad[1][3] + w2 * quad[2][3] + w3 * quad[3][3]);
                }
            }, JExecutionPolicy::J_PARALLEL);
            width = nw;
            height = nh;
            std::cout << "Warning: texture padding to " << nw << " * " << nh <<std::endl;
        }

        int curW = width, curH = height;
        // first level
        textureContainers.push_back(std::make_shared<JSwizzlingTextureContainer>(rawData, curW, curH, channel));

        uchar *previousData = rawData;
        uchar *tmpAlloc = new uchar[curW * curH * channel];
        uchar *currentData = tmpAlloc;
        while(curW > 2) {
            curW /= 2, curH /= 2;
            parallelLoop((size_t)0, (size_t)(curW * curH), [&](const int &index) -> void {
                int yind = index / curW, xind = index % curW;
                uchar &r = currentData[index * channel];
                uchar &g = currentData[index * channel + 1];
                uchar &b = currentData[index * channel + 2];
                uchar &a = currentData[index * channel + 3];
                int ypre = 2 * yind, xpre = 2 * xind;
                int preInd0 = (ypre * 2 * curW + xpre) * channel;
                int preInd1 = (ypre * 2 * curW + xpre +1) * channel;
                int preInd2 = ((ypre + 1) * 2 * curW + xpre) * channel;
                int preInd3 = ((ypre + 1) * 2 * curW + xpre + 1) * channel;
                r = (previousData[preInd0] + previousData[preInd1] + previousData[preInd2] + previousData[preInd3]) * 0.25f;
                g = (previousData[preInd0 + 1] + previousData[preInd1 + 1] + previousData[preInd2 + 1] + previousData[preInd3 + 1]) * 0.25f;
                b = (previousData[preInd0 + 2] + previousData[preInd1 + 2] + previousData[preInd2 + 2] + previousData[preInd3 + 2]) * 0.25f;
                a = (previousData[preInd0 + 3] + previousData[preInd1 + 3] + previousData[preInd2 + 3] + previousData[preInd3 + 3]) * 0.25f;
            }, JExecutionPolicy::J_PARALLEL);
            textureContainers.push_back(std::make_shared<JSwizzlingTextureContainer>(currentData, curW, curH, channel));
            std::swap(currentData, previousData);
        }

        delete [] tmpAlloc;
        tmpAlloc = nullptr;
        //delete [] previousData;
        //previousData = nullptr;
        //delete [] currentData;
        //currentData = nullptr;
        if(reAlloc) {
            delete [] rawData;
            rawData = nullptr;
        }
    }

    void JTexture2D::readPixel(const std::uint16_t &u, const std::uint16_t &v, uchar &r, uchar &g, uchar &b, uchar &a, const int level) const {
        std::uint32_t texel = textureContainers[level]->read(u, v);
        r = (texel >> 24) & 0xFF;
        g = (texel >> 16) & 0xFF;
        b = (texel >>  8) & 0xFF;
        a = (texel >>  0) & 0xFF;
    }

    glm::vec4 JTexture2D::sample(const glm::vec2 &uv, const float &level) const {
        float u = uv.x, v = uv.y;
        //warpping mode
        std::function<void(float&)> impleWrap = [&](float &coor) -> void {
            if(coor < 0 || coor > 1.0f) {
                switch(warpMode) {
                    case JTextureWarpMode::J_REPEAT:
                        coor = coor > 0 ? coor - static_cast<int>(coor) : 1.0f - (static_cast<int>(coor) - coor);
                    break;
                    case JTextureWarpMode::J_CLAMP_TO_EDGE:
                        coor = coor > 0 ? 1.0f : 0.0f;
                    break;
                    case JTextureWarpMode::J_MIRRORED_REPEAT:
                        coor = coor > 0 ? 1.0f - (coor - static_cast<int>(coor)) : static_cast<int>(coor) - coor;
                    break;
                    default:
                        coor = coor > 0 ? coor - static_cast<int>(coor) : 1.0f - (static_cast<int>(coor) - coor);
                    break;
                }
            }
        };
        impleWrap(u);
        impleWrap(v);
        glm::vec4 texel(1.0f);
        if(!enableMipmap) {
            switch(filterMode) {
                case JTextureFilterMode::J_NEAREST:
                    texel = JTexture2DSampler::textureSamplingNearest(textureContainers[0], glm::vec2(u, v));
                    break;
                case JTextureFilterMode::J_LINEAR:
                    texel = JTexture2DSampler::textureSamplingBilinear(textureContainers[0], glm::vec2(u, v));
                    break;
                default:
                    break;
            }
        }
        else { // linear interpolation between two levels
            glm::vec4 texel1(1.0f), texel2(1.0f);
            uint level1 = glm::min(static_cast<uint>(level), static_cast<uint>(textureContainers.size()) - 1);
            uint level2 = glm::min(static_cast<uint>(level) + 1, static_cast<uint>(textureContainers.size()) - 1);
            switch (filterMode) {
                case JTextureFilterMode::J_LINEAR:
                    if(level1 == level2) {
                        texel2 = texel1 = JTexture2DSampler::textureSamplingBilinear(textureContainers[level1], glm::vec2(u, v));
                    }else {
                        texel1 = JTexture2DSampler::textureSamplingBilinear(textureContainers[level1], glm::vec2(u, v));
                        texel2 = JTexture2DSampler::textureSamplingBilinear(textureContainers[level2], glm::vec2(u, v));
                    }
                break;
                case JTextureFilterMode::J_NEAREST:
                    if(level1 == level2) {
                        texel2 = texel1 = JTexture2DSampler::textureSamplingNearest(textureContainers[level1], glm::vec2(u, v));
                    }else {
                        texel1 = JTexture2DSampler::textureSamplingNearest(textureContainers[level1], glm::vec2(u, v));
                        texel2 = JTexture2DSampler::textureSamplingNearest(textureContainers[level2], glm::vec2(u, v));
                    }
                break;
            }
            float frac = level - static_cast<int>(level);
            texel = frac * texel1 + (1.0f - frac) * texel2;
        }
        return texel;
    }

    glm::vec4 JTexture2DSampler::textureSamplingNearest(JTextureContainer::ptr texture, glm::vec2 uv) {
        uchar r, g, b, a = 255;
        texture->read(
            static_cast<std::uint16_t>(uv.x * texture->getWidth() - 1) + 0.5f,
            static_cast<std::uint16_t>(uv.y * texture->getHeight() - 1) + 0.5f,
            r, g, b, a);
        return glm::vec4(r, g, b, a) / 255.0f;
    }

    glm::vec4 JTexture2DSampler::textureSamplingBilinear(JTextureContainer::ptr texture, glm::vec2 uv) {
        uchar r, g, b, a = 255;
        const auto &w = texture->getWidth();
        const auto &h = texture->getHeight();
        float fx = (uv.x * (w - 1)) + 0.5f, fy = (uv.y * (h - 1)) + 0.5f;
        std::uint16_t ix = static_cast<std::uint16_t>(fx), iy = static_cast<std::uint16_t>(fy);
        float xFrac = fx - ix, yFrac = fy - iy;
        texture->read(ix, iy, r, g, b, a);
        glm::vec4 quad0(r, g, b, a);
        texture->read(ix + 1 > w ? ix : ix + 1, iy, r, g, b, a);
        glm::vec4 quad1(r, g, b, a);
        texture->read(ix, iy + 1 > h ? iy : iy + 1, r, g, b, a);
        glm::vec4 quad2(r, g, b, a);
        texture->read(ix + 1 > w ? ix : ix + 1, iy + 1 > h ? iy : iy + 1, r, g, b, a);
        glm::vec4 quad3(r, g, b, a);
        return ((1.0f - xFrac) * (1.0f - yFrac) * quad0 + xFrac * (1.0f - yFrac) * quad1 + (1.0f - xFrac) * yFrac *
                quad2 + xFrac * yFrac * quad3) / 255.0f;
    }



}