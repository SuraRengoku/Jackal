//
// Created by jonas on 2024/6/24.
//

#ifndef JMATHUTILS_H
#define JMATHUTILS_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define _USE_MATH_DEFINES
#include "math.h"

namespace JackalRenderer {
    class JMathUtils final {
    public:
        static glm::mat4 calcViewPortMatrix(int width, int height) {
            glm::mat4 Mat;
            //浮点乘法在硬件层面和指令执行上都比浮点除法更快
            float halfWidth = width * 0.5f;
            float halfHeight = height * 0.5f;
            Mat[0][0] = halfWidth;  Mat[0][1] = 0.0f;           Mat[0][2] = 0.0f; Mat[0][3] = 0.0f;
            Mat[1][0] = 0.0f;       Mat[1][1] = -halfHeight;    Mat[1][2] = 0.0f; Mat[1][3] = 0.0f;
            Mat[2][0] = 0.0f;       Mat[2][1] = 0.0f;           Mat[2][2] = 1.0f; Mat[2][3] = 0.0f;
            Mat[3][0] = halfWidth;  Mat[3][1] = halfHeight;     Mat[3][2] = 0.0f; Mat[3][3] = 0.0f;
            return Mat;
        }

        /**
         * @brief might be transposed
         * @param x the left corner of the viewport
         * @param y the bottom corner of the viewport
         * @param w the width of the viewport
         * @param h the height of the viewport
         * @param n the near clipping value, 0 by default
         * @param f the far clipping value, 1 by default
         * @return Matrix from NDC to Window coordinates
         */
        static glm::mat4 JcalcNDC2Window(int x, int y, int w, int h, int n, int f) {
            glm::mat4 Mat;
            float halfWidth = w * 0.5f;
            float halfHeight = h * 0.5f;
            Mat[0][0] = halfWidth;  Mat[0][1] = 0.0f;       Mat[0][2] = 0.0f;           Mat[0][3] = x + halfWidth;
            Mat[1][0] = 0.0f;       Mat[1][1] = halfHeight; Mat[1][2] = 0.0f;           Mat[1][3] = y + halfHeight;
            Mat[2][0] = 0.0f;       Mat[2][1] = 0.0f;       Mat[2][2] = (f - n) * 0.5f; Mat[2][3] = (f + n) * 0.5f;
            Mat[3][0] = 0.0f;       Mat[3][1] = 0.0f;       Mat[3][2] = 0.0f;           Mat[3][3] = 1.0f;
            return Mat;
        }
        static glm::mat4 calcViewMatrix(glm::vec3 camera, glm::vec3 target, glm::vec3 worldUp) {
            glm::mat4 Mat;
            glm::vec3 f = glm::normalize(camera - target);
            glm::vec3 l = glm::normalize(cross(worldUp, f));
            glm::vec3 u = cross(f, l);
            Mat[0][0] = l.x;                  Mat[0][1] = u.x;                  Mat[0][2] = f.x;                  Mat[0][3] = 0.0f;
            Mat[1][0] = l.y;                  Mat[1][1] = u.y;                  Mat[1][2] = f.y;                  Mat[1][3] = 0.0f;
            Mat[2][0] = l.z;                  Mat[2][1] = u.z;                  Mat[2][2] = f.z;                  Mat[2][3] = 0.0f;
            Mat[3][0] = - dot(l, camera); Mat[3][1] = - dot(u, camera); Mat[3][2] = - dot(f, camera); Mat[3][3] = 1.0f;
            return Mat;
        }
        static glm::mat4 JcalcViewMatrix(glm::vec3 camera, glm::vec3 target, glm::vec3 worldUp) {
            glm::mat4 Mat;
            glm::vec3 f = glm::normalize(camera - target);
            glm::vec3 l = glm::normalize(cross(worldUp, f));
            glm::vec3 u = cross(f, l);
            Mat[0][0] = l.x;  Mat[0][1] = l.y;  Mat[0][2] = l.z;  Mat[0][3] = - dot(l, camera);
            Mat[1][0] = u.x;  Mat[1][1] = u.y;  Mat[1][2] = u.z;  Mat[1][3] = - dot(u, camera);
            Mat[2][0] = f.x;  Mat[2][1] = f.y;  Mat[2][2] = f.z;  Mat[2][3] = - dot(f, camera);
            Mat[3][0] = 0.0f; Mat[3][1] = 0.0f; Mat[3][2] = 0.0f; Mat[3][3] = 1.0f;
            return Mat;
        }
        static glm::mat4 calsPersProjectMatrix(float fovy, float aspect, float near, float far) {
            glm::mat4 Mat;
            float jFovy = fovy * M_PI / 180;
            const float tanHalfFovy = std::tan(jFovy * 0.5f);
            Mat[0][0] = 1.0f / (tanHalfFovy * aspect); Mat[0][1] = 0.0f;               Mat[0][2] = 0.0f;                               Mat[0][3] = 0.0f;
            Mat[1][0] = 0.0f;                          Mat[1][1] = 1.0f / tanHalfFovy; Mat[1][2] = 0.0f;                               Mat[1][3] = 0.0f;
            Mat[2][0] = 0.0f;                          Mat[2][1] = 0.0f;               Mat[2][2] = (far + near) / (near - far);        Mat[2][3] = - 1.0f;
            Mat[3][0] = 0.0f;                          Mat[3][1] = 0.0f;               Mat[3][2] = (2.0f * near * far) / (near - far); Mat[3][3] = 0.0f;
            return Mat;
        }

        /**
         *  @param left, right, top, bottom : coordinates with signs
         *  @param nD, fD : distance of near and far plane without signs
         *  @brief might be transposed
         */
        static glm::mat4 JcalPersProjectMatrix(float left, float right, float top, float bottom, float nD, float fD) {
            glm::mat4 Mat;
            Mat[0][0] = 2.0f * nD / (right - left); Mat[0][1] = 0.0f;                       Mat[0][2] = (right + left) / (right - left); Mat[0][3] = 0.0;
            Mat[1][0] = 0.0f                      ; Mat[1][1] = 2.0f * nD / (top - bottom); Mat[1][2] = (top + bottom) / (top - bottom); Mat[1][3] = 0.0;
            Mat[2][0] = 0.0f                      ; Mat[2][1] = 0.0f;                       Mat[2][2] = (fD + nD) / (nD - fD);           Mat[2][3] = 2.0f * nD * fD / (nD - fD);
            Mat[3][0] = 0.0f                      ; Mat[3][1] = 0.0f;                       Mat[3][2] = - 1.0f;                          Mat[3][3] = 0.0;
            return Mat;
        }
        static glm::mat4 calsOrthoProjectMatrix(float left, float right, float bottom, float top, float near, float far) {
            glm::mat4 Mat;
            Mat[0][0] = 2.0f / (right - left); Mat[0][1] = 0.0f;                  Mat[0][2] = 0.0f;                     Mat[0][3] = 0.0f;
            Mat[1][0] = 0.0f;                  Mat[1][1] = 2.0f / (top - bottom); Mat[1][2] = 0.0f;                     Mat[1][3] = 0.0f;
            Mat[2][0] = 0.0f;                  Mat[2][1] = 0.0f;                  Mat[2][2] = - 2.0f / (far - near);        Mat[2][3] = 0.0f;
            Mat[3][0] = 0.0f;                  Mat[3][1] = 0.0f;                  Mat[3][2] = - (far + near) / (far - near);Mat[3][3] = 1.0f;
            return Mat;
        }
        static glm::mat4 JcalsOrthoProjectMatrix(float left, float right, float bottom, float top, float near, float far) {
            glm::mat4 Mat;
            Mat[0][0] = 2.0f / (right - left); Mat[0][1] = 0.0f;                  Mat[0][2] = 0.0f;                Mat[0][3] = (left + right) / (left - right);
            Mat[1][0] = 0.0f;                  Mat[1][1] = 2.0f / (top - bottom); Mat[1][2] = 0.0f;                Mat[1][3] = (bottom + top) / (bottom - top);
            Mat[2][0] = 0.0f;                  Mat[2][1] = 0.0f;                  Mat[2][2] = 2.0f / (near - far); Mat[2][3] = (near + far) / (near - far);
            Mat[3][0] = 0.0f;                  Mat[3][1] = 0.0f;                  Mat[3][2] = 0.0f;                Mat[3][3] = 1.0f;
            return Mat;
        }
    };
}


#endif //JMATHUTILS_H
