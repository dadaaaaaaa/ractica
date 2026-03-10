#include "../pch.h"
#include "Camera.h"
#include "../Utils/MathUtils.h"

Camera::Camera()
    : position(0.0f, 3.0f, 5.0f),
    targetPosition(0.0f, 0.0f, 0.0f),
    front(0.0f, 0.0f, -1.0f),
    targetFront(0.0f, 0.0f, -1.0f),
    up(0.0f, 1.0f, 0.0f),
    distance(5.0f),
    targetDistance(5.0f),
    height(3.0f),
    smoothness(0.1f),
    minDistance(-1.0f),
    maxDistance(10.0f),
    zoomSpeed(0.5f),
    angle(0.0f),
    targetAngle(0.0f) {
}

void Camera::update(const glm::vec3& target, float deltaTime) {
    float offsetX = sin(glm::radians(targetAngle)) * targetDistance;
    float offsetZ = -cos(glm::radians(targetAngle)) * targetDistance;

    targetPosition = target + glm::vec3(offsetX, height, offsetZ);
    targetFront = glm::normalize(target - targetPosition);

    position = MathUtils::lerp(position, targetPosition, smoothness);
    front = MathUtils::lerp(front, targetFront, smoothness);
    distance = MathUtils::lerp(distance, targetDistance, smoothness);
    angle = MathUtils::lerp(angle, targetAngle, smoothness);
}

void Camera::zoom(float offset) {
    targetDistance -= offset * zoomSpeed;

    if (targetDistance < minDistance) {
        targetDistance = minDistance;
    }
    if (targetDistance > maxDistance) {
        targetDistance = maxDistance;
    }
}

void Camera::rotate(float angleOffset) {
    targetAngle += angleOffset;
}