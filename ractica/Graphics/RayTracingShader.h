#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../Objects/GameObjects.h" 
class RayTracingShader {
private:
    unsigned int m_programID;
    unsigned int m_rayTracingTexture;
    unsigned int m_rayTracingVAO;
    unsigned int m_rayTracingVBO;

    // Compute shader uniform locations
    unsigned int m_widthLoc;
    unsigned int m_heightLoc;
    unsigned int m_frameCountLoc;
    unsigned int m_cameraPosLoc;
    unsigned int m_cameraFrontLoc;
    unsigned int m_cameraUpLoc;

    // Object buffers
    unsigned int m_snakeBuffer;
    unsigned int m_foodBuffer;
    unsigned int m_obstacleBuffer;
    unsigned int m_fenceBuffer;
    unsigned int m_birdBuffer;
    unsigned int m_cloudBuffer;
    unsigned int m_flowerBuffer;

    int m_width;
    int m_height;
    int m_frameCount;

public:
    RayTracingShader();
    ~RayTracingShader();

    void initialize(int width, int height);
    void cleanup();
    void render(const glm::vec3& cameraPos, const glm::vec3& cameraFront, const glm::vec3& cameraUp);
    void updateObjects(const struct GameObjects& objects);

    unsigned int getTexture() const { return m_rayTracingTexture; }
};