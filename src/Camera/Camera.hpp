#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(10.5f), MouseSensitivity(0.1f)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const;

    void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    const glm::vec3 &GetPosition() const { return Position; }
    const glm::vec3 &GetFront() const { return Front; }
    const glm::vec3 &GetUp() const { return Up; }
    const glm::vec3 &GetRight() const { return Right; }

    glm::vec2 WorldToScreen(const glm::vec3 &worldPosition, const glm::mat4 &projection, const glm::mat4 &view, const glm::vec4 &viewport) const;

private:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;

    void updateCameraVectors();
};
