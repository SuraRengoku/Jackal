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
        std::vector<JTextureHolder::ptr>().swap(texHolders);
    }



}