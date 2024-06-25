//
// Created by jonas on 2024/6/25.
//
#include "JTextureHolder.h"
#include "JParallelWrapper.h"

namespace JackalRenderer{
    JTextureHolder::JTextureHolder(std::uint16_t width, std::uint16_t height):
        width(width), height(height), tData(nullptr) {}

    JTextureHolder::~JTextureHolder() { freeTexture(); }

    std::uint32_t JTextureHolder::read(const std::uint16_t &x, const std::uint16_t &y) const {
        return tData[xyToIndex(x, y)];
    }

    void JTextureHolder::read(const std::uint16_t &x, const std::uint16_t &y,
        uchar &r, uchar &g, uchar &b, uchar &a) const {
        std::uint32_t texel = read(x, y);
        r = (texel >> 24) & 0xFF; // front 8 bits
        g = (texel >> 16) & 0xFF; // second 8 bits
        b = (texel >>  8) & 0xFF; // third 8 bits
        a = (texel >>  0) & 0xFF; // last 8 bits
    }

    void JTextureHolder::loadTexture(const unsigned int &nElements, uchar *data,
        const std::uint16_t &width, const std::uint16_t &height, const int &channel) {
        tData = new std::uint32_t[nElements]; //apply memory
        parallelLoop((size_t)0, (size_t)(height * width), [&](const int &ind) -> void {
            int y = ind / width, x = ind % width;
            uchar r, g, b, a;
            int address = ind * channel;
            switch (channel) {
                case 1:
                    r = g = b = data[address], a = 255;
                    break;
                case 3: //RGB
                    r = data[address], g = data[address + 1], b = data[address + 2], a = 255;
                    break;
                case 4:
                    r = data[address], g = data[address + 1], b = data[address + 2], a = data[address + 3];
                    break;
                default:
                    r = g = b = data[address], a = 255;
                    break;
            }
            tData[xyToIndex(x, y)] = (r << 24) | (g << 16) | (b << 8) | (a << 0);
        }, JExecutionPolicy::J_PARALLEL);
    }

    void JTextureHolder::freeTexture() {
        if(tData) {
            delete [] tData;
            tData = nullptr;
        }
    }

    JLinearTextureHolder::JLinearTextureHolder(uchar *data, std::uint16_t width, std::uint16_t height, int channel)
        : JTextureHolder(width, height) {
        JLinearTextureHolder::loadTexture(width * height, data, width, height, channel);
    }

    uint JLinearTextureHolder::xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const {
        return y * width + x; //reset
    }

    JTilingTextureHolder::JTilingTextureHolder(uchar *data, std::uint16_t width, std::uint16_t height, int channel)
        : JTextureHolder(width, height) {
            widthInTiles = (width + tileBase4 -1) / tileBase4; // upper bound
            heightInTiles = (height + tileBase4 -1) / tileBase4;
            uint nElements = widthInTiles * heightInTiles * tileBase16; // tileBase16 = tileBase4 * tileBase4
            JTilingTextureHolder::loadTexture(nElements, data, width, height, channel);
    }

    uint JTilingTextureHolder::xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const {
        // Refs: https://fgiesen.wordpress.com/2011/01/17/texture-tiling-and-swizzling/
        return (((int)(y >> 2) * widthInTiles + (int)(x >> 2)) << 4) + ((y & 3) << 2) + (x & 3);
    }

    JSwizzlingTextureHolder::JSwizzlingTextureHolder(uchar *data, std::uint16_t width, std::uint16_t height, int channel)
        : JTextureHolder(width, height) {
        widthInTiles = (width + tileBase32 - 1) / tileBase32;
        heightInTiles = (height + tileBase32 -1) / tileBase32;
        uint nElements = widthInTiles * heightInTiles * tileBase1024;
        JSwizzlingTextureHolder::loadTexture(nElements, data, width, height, channel);
    }

    uint JSwizzlingTextureHolder::xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const {
        std::uint8_t rx = x & (tileBase32 - 1), ry = y & (tileBase32 -1);
        std::uint16_t ri = 0;
        JSwizzlingTextureHolder::encodeMortonCurve(rx, ry, ri);
        // ri can also refer to the order of being stored
        return ((y >> bits32) * widthInTiles + (x >> bits32)) * tileBase1024 + ri;
    }

}