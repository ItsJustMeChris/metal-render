#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL.h>

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
    // Constructor with default values
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = 0.0f,
           float fov = 45.0f, float nearPlane = 0.1f, float farPlane = 100.0f)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(10.5f), MouseSensitivity(0.1f),
          FOV(fov), NearPlane(nearPlane), FarPlane(farPlane)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Getter and setter for FOV
    float GetFOV() const { return FOV; }
    void SetFOV(float fov) { FOV = fov; }

    // Getter and setter for near and far planes
    float GetNearPlane() const { return NearPlane; }
    void SetNearPlane(float nearPlane) { NearPlane = nearPlane; }

    float GetFarPlane() const { return FarPlane; }
    void SetFarPlane(float farPlane) { FarPlane = farPlane; }

    // Modify GetProjectionMatrix to only accept aspect ratio

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    const glm::vec3 &GetPosition() const { return Position; }
    const glm::vec3 &GetFront() const { return Front; }
    const glm::vec3 &GetUp() const { return Up; }
    const glm::vec3 &GetRight() const { return Right; }

    glm::vec2 WorldToScreen(const glm::vec3 &worldPosition, const glm::mat4 &projection, const glm::mat4 &view, const glm::vec4 &viewport) const;
    const void Teleport(const glm::vec3 &position) { Position = position; }
    void LookAt(const glm::vec3 &targetPosition);

    void OnMouseButtonDown(int x, int y);
    void OnMouseButtonUp();
    void OnMouseMove(int x, int y);
    void ProcessKeyboardInput(const Uint8 *state, float deltaTime);

private:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float FOV;
    float NearPlane;
    float FarPlane;

    float MovementSpeed;
    float MouseSensitivity;

    bool mouseButtonDown = false;
    int lastMouseX = 0, lastMouseY = 0;
    bool firstMouse = true;

    void updateCameraVectors();
};
