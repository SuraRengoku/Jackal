//
// Created by jonas on 2024/9/10.
//
#include "JDrawableMesh.h"
#include <map>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "JTexture2D.h"
#include "JShadingPipeline.h"

using std::map;

namespace JackalRenderer {
    JDrawableSubMesh::JDrawableSubMesh(const JDrawableSubMesh &mesh)
        : vertices(mesh.vertices), indices(mesh.indices),drawingMaterial(mesh.drawingMaterial) {}

    JDrawableSubMesh& JDrawableSubMesh::operator=(const JDrawableSubMesh& mesh) {
        if(&mesh == this)
            return *this;
        this -> vertices = mesh.vertices;
        this -> indices = mesh.indices;
        this -> drawingMaterial = mesh.drawingMaterial;
        return *this;
    }

    void JDrawableSubMesh::clear() {
        /*
         * 1. 创建一个新的空的vector<T>
         * 2. 将该空vector与原有的vector交换
         * 3. 原有的vector被清空
         * 4. 当作用域结束时，临时的空vector被释放
         */
        vector<JVertex>().swap(this -> vertices);
        vector<uint>().swap(this -> indices);
    }

    // ref: https://learnopengl.com/Model-Loading/Assimp
    class AssimpImporterWrapper final {
    public:
        map<string, int> textureDict = {};
        string directory = "";
        bool generatedMipmap = false;

        JDrawableSubMesh procesMesh(aiMesh *mesh, const aiScene *scene) {
            JDrawableSubMesh drawble;

            vector<JVertex> vertices;
            vector<uint> indices;

            for(uint i = 0; i < mesh -> mNumVertices; ++i) {
                JVertex vertex;
                vertex.vpostions = glm::vec3(mesh -> mVertices[i].x, mesh -> mVertices[i].y, mesh -> mVertices[i].z);
                if(mesh -> HasNormals())
                        vertex.vnormals = glm::vec3(mesh -> mNormals[i].x, mesh -> mNormals[i].y, mesh -> mNormals[i].z);
                if(mesh -> mTextureCoords[0]) {
                    vertex.vtexcoords = glm::vec2(mesh -> mTextureCoords[0][i].x, mesh -> mTextureCoords[0][i].y);
                    vertex.vtangent = glm::vec3(mesh -> mTangents[i].x, mesh -> mTangents[i].y, mesh -> mTangents[i].z);
                    vertex.vbitanget = glm::vec3(mesh -> mBitangents[i].x, mesh -> mBitangents[i].y, mesh -> mBitangents[i].z);
                }else
                    vertex.vtexcoords = glm::vec2(0.0f, 0.0f);
                vertices.push_back(vertex);
            }

            for(uint i = 0; i < mesh -> mNumFaces; ++i) {
                aiFace face = mesh -> mFaces[i];
                for(uint j = 0; j < face.mNumIndices; ++j)
                    indices.push_back(face.mIndices[j]);
            }

            aiMaterial* material = scene -> mMaterials[mesh -> mMaterialIndex];

            auto loadFunc = [&](aiTextureType type) -> int {
                for(int i = 0; i < material -> GetTextureCount(type); ++i) {
                    aiString str;
                    material -> GetTexture(type, i, &str);
                    if(textureDict.find(str.C_Str()) != textureDict.end())
                        return textureDict[str.C_Str()];
                    else {
                        JTexture2D::ptr diffTex = std::make_shared<JTexture2D>(generatedMipmap);
                        bool success = diffTex -> loadTextureFromFile(directory + '/' + str.C_Str());
                        auto texId = JShadingPipeline::uploadTexture2D(diffTex);
                        // texId 是插入纹理后纹理容器的末尾下标
                        textureDict.insert({str.C_Str(), texId});
                        return texId;
                    }
                }
                return -1;
            };

            drawble.setDiffuseMapTexId(loadFunc(aiTextureType_DIFFUSE));
            drawble.setSpecularMapTexId(loadFunc(aiTextureType_SPECULAR));
            drawble.setNormalMapTexId(loadFunc(aiTextureType_HEIGHT));
            drawble.setGlowMapTexId(loadFunc(aiTextureType_EMISSIVE));

            drawble.setVertices(vertices);
            drawble.setIndices(indices);

            return drawble;
        }

        void processNode(aiNode* node, const aiScene* scene, vector<JDrawableSubMesh>& drawables) {
            for(uint i = 0; i < node -> mNumMeshes; ++i) {
                aiMesh* mesh = scene -> mMeshes[node -> mMeshes[i]];
                drawables.push_back(procesMesh(mesh, scene));
            }
            for(uint i = 0; i < node -> mNumChildren; ++i) //recursive
                processNode(node -> mChildren[i], scene, drawables);
        }
    };

    void JDrawableMesh::importMeshFromFile(const string& path, bool generatedMipmap) {
        for(auto &drawable : drawables)
            drawable.clear();
        drawables.clear();

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_SplitLargeMeshes | aiProcess_FixInfacingNormals);
        if(!scene || scene -> mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene -> mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            exit(1);
        }
        AssimpImporterWrapper wrapper;
        wrapper.generatedMipmap = generatedMipmap;
        wrapper.directory = path.substr(0, path.find_last_of('/'));
        wrapper.processNode(scene -> mRootNode, scene, drawables);
    }

    void JDrawableMesh::clear() {
        for(auto &drawable : drawables)
            drawable.clear();
    }

    JDrawableMesh::JDrawableMesh(const string &path, bool generatedMipmap) {
        importMeshFromFile(path, generatedMipmap);
    }

    uint JDrawableMesh::getDrawableMaxFaceNums() const {
        uint num = 0;
        for(const auto &drawable : drawables)
            num = std::max(num, (uint)drawable.getIndices().size() / 3);
        return num;
    }
}