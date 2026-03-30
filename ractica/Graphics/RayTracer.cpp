#include "../pch.h"
#include "RayTracer.h"
#include <random>
#include <glm/gtc/random.hpp>

RayTracer::RayTracer()
    : m_enabled(false)
    , m_samplesPerPixel(4)
    , m_maxDepth(3) {
}

glm::vec3 RayTracer::traceRay(const Ray& ray, const std::function<HitInfo(const Ray&)>& intersectCallback, int depth) {
    if (depth >= m_maxDepth) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    HitInfo hit = intersectCallback(ray);

    if (!hit.hit) {
        // Возвращаем цвет неба/окружения
        float t = 0.5f * (ray.direction.y + 1.0f);
        return glm::vec3(0.53f, 0.81f, 0.92f) * (1.0f - t) + glm::vec3(0.8f, 0.9f, 1.0f) * t;
    }

    // Простое освещение с одной точкой света
    glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
    glm::vec3 lightColor(1.0f, 0.95f, 0.85f);

    glm::vec3 ambient = hit.color * 0.3f;
    glm::vec3 diffuse = glm::vec3(0.0f);

    // Проверка на тень
    glm::vec3 lightDir = glm::normalize(lightPos - hit.point);
    Ray shadowRay(hit.point + hit.normal * 0.001f, lightDir);
    HitInfo shadowHit = intersectCallback(shadowRay);

    if (!shadowHit.hit || shadowHit.distance > glm::length(lightPos - hit.point)) {
        float diff = glm::max(glm::dot(hit.normal, lightDir), 0.0f);
        diffuse = hit.color * lightColor * diff;
    }

    // Простое зеркальное отражение
    glm::vec3 specular(0.0f);
    glm::vec3 viewDir = glm::normalize(-ray.direction);
    glm::vec3 reflectDir = glm::reflect(-lightDir, hit.normal);
    float spec = glm::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), 32.0f);
    specular = lightColor * spec * 0.5f;

    // Рекурсивное отражение
    glm::vec3 reflection(0.0f);
    if (depth < m_maxDepth - 1) {
        glm::vec3 reflectDir = glm::reflect(ray.direction, hit.normal);
        Ray reflectRay(hit.point + hit.normal * 0.001f, reflectDir);
        reflection = traceRay(reflectRay, intersectCallback, depth + 1) * 0.3f;
    }

    return ambient + diffuse + specular + reflection;
}

Ray RayTracer::getRayFromCamera(const glm::vec3& cameraPos, const glm::vec3& cameraDir,
    const glm::vec3& up, float fov, float aspect, float x, float y) {
    // Нормализуем направления
    glm::vec3 forward = glm::normalize(cameraDir);
    glm::vec3 upNorm = glm::normalize(up);

    // Правильный порядок для получения правого вектора
    glm::vec3 right = glm::normalize(glm::cross(forward, upNorm));
    glm::vec3 realUp = glm::normalize(glm::cross(right, forward));

    // Вычисляем размеры области проецирования
    float tanHalfFov = tan(glm::radians(fov * 0.5f));
    float viewportHeight = 2.0f * tanHalfFov;
    float viewportWidth = viewportHeight * aspect;

    // x и y уже в диапазоне [-1, 1]
    glm::vec3 rayDir = forward
        + (x * viewportWidth) * right
        + (y * viewportHeight) * realUp;
    rayDir = glm::normalize(rayDir);

    return Ray(cameraPos, rayDir);
}