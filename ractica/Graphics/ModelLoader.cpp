#include "../pch.h"
#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem>

extern std::string g_modelsPath;
extern std::string g_texturesPath;

// Вспомогательная функция для извлечения имени файла из пути
std::string extractFilename(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

// Функция для загрузки текстуры из файла
GLuint loadTextureFromFile(const std::string& path) {
    std::cout << "  Loading texture from file: " << path << std::endl;

    if (!std::filesystem::exists(path)) {
        std::cout << "  ✗ Texture file not found" << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

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

        std::cout << "  ✓ Texture loaded: " << width << "x" << height << std::endl;
        return textureID;
    }

    std::cout << "  ✗ Failed to load texture: " << stbi_failure_reason() << std::endl;
    stbi_image_free(data);
    glDeleteTextures(1, &textureID);
    return 0;
}

// Функция для загрузки текстуры из данных Assimp
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
            return textureID;
        }
        else {
            std::cout << "  ✗ Failed to decode embedded texture" << std::endl;
            glDeleteTextures(1, &textureID);
            return 0;
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
        return textureID;
    }
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

    // Получаем имя модели без расширения
    std::string baseName = filename;
    size_t dotPos = baseName.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseName = baseName.substr(0, dotPos);
    }
    std::cout << "Model base name: " << baseName << std::endl;

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
        material.textureID = 0;

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

        // Пытаемся загрузить диффузную текстуру из материала
        aiString texPath;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string textureFile = texPath.C_Str();
            std::cout << "  Diffuse texture from material: " << textureFile << std::endl;

            // Извлекаем только имя файла
            textureFile = extractFilename(textureFile);
            std::cout << "  Extracted filename: " << textureFile << std::endl;

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

                if (std::filesystem::exists(textureFullPath)) {
                    material.textureID = loadTextureFromFile(textureFullPath);
                }
                else {
                    // Пробуем в папке текстур
                    textureFullPath = g_texturesPath + textureFile;
                    if (std::filesystem::exists(textureFullPath)) {
                        material.textureID = loadTextureFromFile(textureFullPath);
                    }
                }
            }

            if (material.textureID != 0) {
                material.texturePath = textureFile;
                std::cout << "  ✓ Texture loaded successfully!" << std::endl;
            }
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

    // Загружаем меши с правильной индексацией
    std::cout << "\n--- Loading Meshes ---" << std::endl;
    int totalVertices = 0;
    int totalTriangles = 0;

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "Mesh " << i << ": " << mesh->mNumVertices << " vertices, "
            << mesh->mNumFaces << " faces" << std::endl;

        int materialIndex = mesh->mMaterialIndex;
        if (materialIndex >= (int)model.materials.size()) {
            std::cout << "  Warning: Material index " << materialIndex
                << " out of range, using 0" << std::endl;
            materialIndex = 0;
        }

        // Для каждой грани (треугольника)
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];

            if (face.mNumIndices != 3) {
                std::cout << "  Warning: Face with " << face.mNumIndices << " indices (not triangle)" << std::endl;
                continue;
            }

            // Добавляем индекс материала для этого треугольника
            model.materialIndices.push_back(materialIndex);

            // Для каждой вершины треугольника
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                unsigned int vertexIdx = face.mIndices[k];

                // Вершины
                model.vertices.push_back(mesh->mVertices[vertexIdx].x);
                model.vertices.push_back(mesh->mVertices[vertexIdx].y);
                model.vertices.push_back(mesh->mVertices[vertexIdx].z);

                // Нормали
                if (mesh->HasNormals()) {
                    model.normals.push_back(mesh->mNormals[vertexIdx].x);
                    model.normals.push_back(mesh->mNormals[vertexIdx].y);
                    model.normals.push_back(mesh->mNormals[vertexIdx].z);
                }

                // Текстурные координаты
                if (mesh->HasTextureCoords(0)) {
                    model.texCoords.push_back(mesh->mTextureCoords[0][vertexIdx].x);
                    model.texCoords.push_back(mesh->mTextureCoords[0][vertexIdx].y);
                }

                totalVertices++;
            }
            totalTriangles++;
        }
    }

    // Если нормалей нет, генерируем простые
    if (model.normals.empty()) {
        std::cout << "\nGenerating default normals" << std::endl;
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            model.normals.push_back(0.0f);
            model.normals.push_back(1.0f);
            model.normals.push_back(0.0f);
        }
    }

    // Если текстурных координат нет, добавляем нулевые
    if (model.texCoords.empty()) {
        std::cout << "Generating default texcoords" << std::endl;
        for (size_t i = 0; i < model.vertices.size() / 3; i++) {
            model.texCoords.push_back(0.0f);
            model.texCoords.push_back(0.0f);
        }
    }

    // ПРИНУДИТЕЛЬНАЯ ЗАГРУЗКА ТЕКСТУР ДЛЯ ПОЛА (как в редакторе)
    if (subFolder == "floor") {
        std::cout << "\n--- Forced texture search for FLOOR ---" << std::endl;

        std::string basePath = fullPath.substr(0, fullPath.find_last_of("\\/") + 1);

        // Список всех возможных текстур для пола
        std::vector<std::string> possibleTextures = {
            // С суффиксами
            baseName + "_diffuse.jpg",
            baseName + "_diffuse.png",
            baseName + "_diffuse.tga",
            baseName + "_color.jpg",
            baseName + "_color.png",
            baseName + "_albedo.jpg",
            baseName + "_albedo.png",
            baseName + "_basecolor.jpg",
            baseName + "_basecolor.png",
            baseName + "_col.jpg",
            baseName + "_col.png",

            // Без суффиксов
            baseName + ".jpg",
            baseName + ".png",
            baseName + ".tga",
            baseName + ".bmp",

            // Специфичные для StoneFloor
            "StoneFloor_diffuse.jpg",
            "StoneFloor_diffuse.png",
            "StoneFloor_Normal.jpg",
            "StoneFloor_Normal.png",
            "StoneFloor_Specular.jpg",
            "StoneFloor_Specular.png",
            "StoneFloor_BaseColor.jpg",
            "StoneFloor_BaseColor.png",
            "StoneFloor_Albedo.jpg",
            "StoneFloor_Albedo.png",
            "StoneFloor_Color.jpg",
            "StoneFloor_Color.png",
            "StoneFloor_Diffuse.jpg",
            "StoneFloor_Diffuse.png",

            // Общие названия
            "diffuse.jpg",
            "diffuse.png",
            "albedo.jpg",
            "albedo.png",
            "color.jpg",
            "color.png",
            "basecolor.jpg",
            "basecolor.png",
            "col.jpg",
            "col.png",
            "texture.jpg",
            "texture.png",
            "floor.jpg",
            "floor.png",
            "floor_diffuse.jpg",
            "floor_diffuse.png"
        };

        GLuint loadedTextureID = 0;
        std::string loadedTextureName;

        // Ищем текстуру в папке с моделью
        for (const auto& texName : possibleTextures) {
            std::string texPath = basePath + texName;
            if (std::filesystem::exists(texPath)) {
                std::cout << "  Found texture: " << texName << std::endl;
                loadedTextureID = loadTextureFromFile(texPath);
                if (loadedTextureID) {
                    std::cout << "  ✓ Texture loaded successfully! ID: " << loadedTextureID << std::endl;
                    loadedTextureName = texName;
                    break;
                }
            }
        }

        // Если не нашли в папке модели, ищем в папке текстур
        if (loadedTextureID == 0) {
            for (const auto& texName : possibleTextures) {
                std::string texPath = g_texturesPath + texName;
                if (std::filesystem::exists(texPath)) {
                    std::cout << "  Found texture in textures folder: " << texName << std::endl;
                    loadedTextureID = loadTextureFromFile(texPath);
                    if (loadedTextureID) {
                        std::cout << "  ✓ Texture loaded successfully! ID: " << loadedTextureID << std::endl;
                        loadedTextureName = texName;
                        break;
                    }
                }
            }
        }

        // Если текстура найдена, применяем её ко всем материалам
        if (loadedTextureID != 0) {
            std::cout << "  Applying floor texture to all " << model.materials.size() << " materials" << std::endl;
            for (auto& material : model.materials) {
                // Если у материала уже есть текстура, не заменяем
                if (material.textureID == 0) {
                    material.textureID = loadedTextureID;
                    material.texturePath = loadedTextureName;
                }
            }
        }
        else {
            std::cout << "  ✗ No floor texture found!" << std::endl;

            // Выводим список файлов в папке для отладки
            std::cout << "  Files in model folder:" << std::endl;
            try {
                for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
                    std::cout << "    - " << entry.path().filename().string() << std::endl;
                }
            }
            catch (...) {
                std::cout << "    Could not list directory" << std::endl;
            }
        }
    }

    std::cout << "\nTotal vertices loaded: " << model.vertices.size() / 3 << std::endl;
    std::cout << "Total triangles: " << totalTriangles << std::endl;

    model.loaded = (model.vertices.size() > 0);

    if (model.loaded) {
        std::cout << "✅ MODEL LOADED SUCCESSFULLY IN GAME!" << std::endl;

        // Подсчитываем загруженные текстуры
        int texturesLoaded = 0;
        for (const auto& mat : model.materials) {
            if (mat.textureID != 0) texturesLoaded++;
        }
        std::cout << "Textures loaded: " << texturesLoaded << "/" << model.materials.size() << std::endl;
    }

    std::cout << "========================================\n" << std::endl;
    return model.loaded;
}