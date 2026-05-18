#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <functional>

// RayTracer.h - добавить поле

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float maxDistance = 100.0f;  // <-- ƒќЅј¬»“№

    Ray() : origin(0.0f), direction(0.0f, 0.0f, -1.0f), maxDistance(100.0f) {}
    Ray(const glm::vec3& o, const glm::vec3& d)
        : origin(o), direction(glm::normalize(d)), maxDistance(100.0f) {
    }
    Ray(const glm::vec3& o, const glm::vec3& d, float maxDist)
        : origin(o), direction(glm::normalize(d)), maxDistance(maxDist) {
    }

    glm::vec3 pointAt(float t) const { return origin + direction * t; }
};

struct HitInfo {
    bool hit;
    float distance;
    glm::vec3 point;
    glm::vec3 normal;
    glm::vec3 color;

    HitInfo() : hit(false), distance(0.0f), point(0.0f), normal(0.0f), color(0.0f) {}
};

class RayTracer {
private:
    bool m_enabled;
    int m_samplesPerPixel;
    int m_maxDepth;

public:
    RayTracer();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void setSamplesPerPixel(int samples) { m_samplesPerPixel = samples; }
    int getSamplesPerPixel() const { return m_samplesPerPixel; }  // ƒќЅј¬Ћя≈ћ
    void setMaxDepth(int depth) { m_maxDepth = depth; }
    int getMaxDepth() const { return m_maxDepth; }  // ƒќЅј¬Ћя≈ћ

    // ќсновной метод трассировки
    glm::vec3 traceRay(const Ray& ray, const std::function<HitInfo(const Ray&)>& intersectCallback, int depth = 0);

    // ћетод дл€ генерации луча из камеры
    Ray getRayFromCamera(const glm::vec3& cameraPos, const glm::vec3& cameraDir,
        const glm::vec3& up, float fov, float aspect, float x, float y);
   // int index = ((height - 1 - y) * width + x) * 3;

    // ќсвещение
    glm::vec3 calculateLighting(const HitInfo& hit, const glm::vec3& lightPos,
        const glm::vec3& lightColor, const std::function<HitInfo(const Ray&)>& intersectCallback);
};