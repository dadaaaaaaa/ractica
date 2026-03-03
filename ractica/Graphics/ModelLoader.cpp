#include "../pch.h"
#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>

extern std::string g_modelsPath;

bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder) {
    std::cout << "\n========== FBX LOADER IN GAME ==========" << std::endl;
    std::cout << "Loading model for folder: " << subFolder << std::endl;
    std::cout << "Filename: " << filename << std::endl;

    // Формируем полный путь
    std::string fullPath = g_modelsPath + subFolder + "\\" + filename;
    std::cout << "Full path: " << fullPath << std::endl;

    // Проверяем существование файла
    if (!std::filesystem::exists(fullPath)) {
        std::cout << "❌ ERROR: File does not exist!" << std::endl;
        return false;
    }

    std::cout << "Initializing Assimp importer..." << std::endl;
    Assimp::Importer importer;

    // Загружаем сцену
    const aiScene* scene = importer.ReadFile(fullPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace);

    if (!scene) {
        std::cout << "❌ Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::cout << "✓ Scene loaded successfully!" << std::endl;
    std::cout << "  - Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  - Materials: " << scene->mNumMaterials << std::endl;

    // Очищаем старые данные
    model.vertices.clear();
    model.normals.clear();
    model.texCoords.clear();
    model.materials.clear();
    model.materialIndices.clear();

    // Загружаем материалы
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material material;

        aiColor3D color(1.0f, 1.0f, 1.0f);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse = glm::vec3(color.r, color.g, color.b);

        model.materials.push_back(material);
    }

    // Если нет материалов, создаем дефолтный
    if (model.materials.empty()) {
        Material defaultMat;
        defaultMat.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        model.materials.push_back(defaultMat);
    }

    // Загружаем меши
    int totalVertices = 0;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        int materialIndex = mesh->mMaterialIndex;
        if (materialIndex >= (int)model.materials.size()) {
            materialIndex = 0;
        }

        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            // Вершины
            model.vertices.push_back(mesh->mVertices[j].x);
            model.vertices.push_back(mesh->mVertices[j].y);
            model.vertices.push_back(mesh->mVertices[j].z);

            // Нормали
            if (mesh->HasNormals()) {
                model.normals.push_back(mesh->mNormals[j].x);
                model.normals.push_back(mesh->mNormals[j].y);
                model.normals.push_back(mesh->mNormals[j].z);
            }
            else {
                model.normals.push_back(0.0f);
                model.normals.push_back(1.0f);
                model.normals.push_back(0.0f);
            }

            // Текстурные координаты
            if (mesh->HasTextureCoords(0)) {
                model.texCoords.push_back(mesh->mTextureCoords[0][j].x);
                model.texCoords.push_back(mesh->mTextureCoords[0][j].y);
            }
            else {
                model.texCoords.push_back(0.0f);
                model.texCoords.push_back(0.0f);
            }
        }

        // Индексы материалов для каждого треугольника
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            if (face.mNumIndices == 3) {
                model.materialIndices.push_back(materialIndex);
            }
        }

        totalVertices += mesh->mNumVertices;
    }

    std::cout << "Total vertices loaded: " << totalVertices << std::endl;
    std::cout << "Total triangles: " << model.materialIndices.size() << std::endl;

    model.loaded = (model.vertices.size() > 0);

    if (model.loaded) {
        std::cout << "✅ MODEL LOADED SUCCESSFULLY IN GAME!" << std::endl;
    }

    std::cout << "========================================\n" << std::endl;
    return model.loaded;
}