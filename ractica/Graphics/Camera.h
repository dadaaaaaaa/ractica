#pragma once
#include <glm/glm.hpp>

class Camera {
private:
    glm::vec3 position;
    glm::vec3 targetPosition;
    glm::vec3 front;
    glm::vec3 targetFront;
    glm::vec3 up;
    float distance;
    float targetDistance;
    float height;
    float smoothness;
    float minDistance;
    float maxDistance;
    float zoomSpeed;
    float angle;
    float targetAngle;

public:
    Camera();

    void update(const glm::vec3& target, float deltaTime);
    void zoom(float offset);
    void rotate(float angleOffset);

    // Геттеры
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    float getTargetDistance() const { return targetDistance; }

    // Сеттеры
    void setHeight(float newHeight) { height = newHeight; }
    void setSmoothness(float newSmoothness) { smoothness = newSmoothness; }
    void setMinDistance(float minDist) { minDistance = minDist; }
    void setMaxDistance(float maxDist) { maxDistance = maxDist; }
    void setZoomSpeed(float speed) { zoomSpeed = speed; }
    void setTargetDistance(float distance) { targetDistance = distance; }
};