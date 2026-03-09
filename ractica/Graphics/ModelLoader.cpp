#include "../pch.h"
#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>

extern std::string g_modelsPath;
extern std::string g_texturesPath;

// Вспомогательная функция для загрузки текстуры из данных Assimp
GLuint loadTextureFromAssimp(const aiTexture* texture, const std::string& filename) {
    GLuint textureID = 0;
    glGenTextures(1, &textureID);

    if (texture->mHeight == 0) {
        // Сжатая текстура (например, PNG встроенный в FBX)
        std::cout << "  Loading embedded compressed texture: " << filename << std::endl;

        int width, height, channels;
        unsigned char* data = stbi_load_from_memory(
            reinterpret_cast<unsigned char*>(texture->pcData),
            texture->mWidth,
            &width, &height, &channels, 0
        );

        if (data) {
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "  ✓ Embedded texture loaded: " << width << "x" << height << std::endl;
        }
        else {
            std::cout << "  ✗ Failed to decode embedded texture" << std::endl;
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }
    else {
        // Несжатая текстура (raw data)
        std::cout << "  Loading embedded raw texture: " << filename << std::endl;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->mWidth, texture->mHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, texture->pcData);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        std::cout << "  ✓ Embedded raw texture loaded: " << texture->mWidth << "x" << texture->mHeight << std::endl;
    }

    return textureID;
}

// Функция для загрузки текстуры из файла
GLuint loadTextureFromFile(const std::string& path) {
    std::cout << "  Loading texture from file: " << path << std::endl;

    if (!std::filesystem::exists(path)) {
        std::cout << "  ✗ Texture file not found: " << path << std::endl;
        return 0;
    }

    GLuint textureID = 0;
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (data) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "  ✓ Texture loaded: " << width << "x" << height << std::endl;
    }
    else {
        std::cout << "  ✗ Failed to load texture: " << stbi_failure_reason() << std::endl;
    }

    return textureID;
}

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

    // Загружаем сцену с флагами для текстур
    const aiScene* scene = importer.ReadFile(fullPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_FindInstances |
        aiProcess_OptimizeMeshes);

    if (!scene) {
        std::cout << "❌ Assimp error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::cout << "✓ Scene loaded successfully!" << std::endl;
    std::cout << "  - Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "  - Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "  - Textures: " << scene->mNumTextures << std::endl;

    // Очищаем старые данные
    model.vertices.clear();
    model.normals.clear();
    model.texCoords.clear();
    model.materials.clear();
    model.materialIndices.clear();

    // Загружаем материалы и текстуры
    std::cout << "\n--- Loading Materials & Textures ---" << std::endl;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material material;

        aiString name;
        mat->Get(AI_MATKEY_NAME, name);
        std::cout << "\nMaterial " << i << ": " << name.C_Str() << std::endl;

        // Загружаем диффузный цвет
        aiColor3D color(1.0f, 1.0f, 1.0f);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse = glm::vec3(color.r, color.g, color.b);
        std::cout << "  Diffuse color: (" << color.r << ", " << color.g << ", " << color.b << ")" << std::endl;

        // Загружаем specular цвет
        aiColor3D specular(0.0f, 0.0f, 0.0f);
        mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        material.specular = glm::vec3(specular.r, specular.g, specular.b);

        // Загружаем shininess
        float shininess = 32.0f;
        mat->Get(AI_MATKEY_SHININESS, shininess);
        material.shininess = shininess;

        // Пытаемся загрузить диффузную текстуру
        aiString texPath;
        material.textureID = 0;

        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string textureFile = texPath.C_Str();
            std::cout << "  Diffuse texture: " << textureFile << std::endl;

            // Проверяем, встроенная ли текстура
            if (textureFile[0] == '*') {
                // Встроенная текстура (индекс в массиве текстур)
                int texIndex = std::stoi(textureFile.substr(1));
                if (texIndex >= 0 && texIndex < scene->mNumTextures) {
                    std::cout << "  Loading embedded texture at index: " << texIndex << std::endl;
                    material.textureID = loadTextureFromAssimp(scene->mTextures[texIndex], textureFile);
                }
            }
            else {
                // Внешняя текстура - ищем рядом с моделью
                std::string basePath = fullPath.substr(0, fullPath.find_last_of("\\/") + 1);
                std::string textureFullPath = basePath + textureFile;

                // Также пробуем в папке текстур
                if (!std::filesystem::exists(textureFullPath)) {
                    textureFullPath = g_texturesPath + textureFile;
                }

                material.textureID = loadTextureFromFile(textureFullPath);
            }

            if (material.textureID != 0) {
                material.texturePath = textureFile;
                std::cout << "  ✓ Texture loaded successfully!" << std::endl;
            }
            else {
                std::cout << "  ✗ Failed to load texture" << std::endl;
            }
        }
        else {
            std::cout << "  No diffuse texture found" << std::endl;
        }

        model.materials.push_back(material);
    }

    // Если нет материалов, создаем дефолтный
    if (model.materials.empty()) {
        std::cout << "\nNo materials found, creating default material" << std::endl;
        Material defaultMat;
        defaultMat.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
        defaultMat.specular = glm::vec3(1.0f, 1.0f, 1.0f);
        defaultMat.shininess = 32.0f;
        defaultMat.textureID = 0;
        model.materials.push_back(defaultMat);
    }

    // Загружаем меши
    std::cout << "\n--- Loading Meshes ---" << std::endl;
    int totalVertices = 0;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "Mesh " << i << ": " << mesh->mNumVertices << " vertices" << std::endl;

        int materialIndex = mesh->mMaterialIndex;
        if (materialIndex >= (int)model.materials.size()) {
            std::cout << "  Warning: Material index " << materialIndex
                << " out of range, using 0" << std::endl;
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

    std::cout << "\nTotal vertices loaded: " << totalVertices << std::endl;
    std::cout << "Total triangles: " << model.materialIndices.size() << std::endl;

    model.loaded = (model.vertices.size() > 0);

    if (model.loaded) {
        std::cout << "✅ MODEL LOADED SUCCESSFULLY IN GAME!" << std::endl;
    }

    std::cout << "========================================\n" << std::endl;
    return model.loaded;
}