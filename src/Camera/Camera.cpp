#include "Camera.hpp"

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(FOV), aspectRatio, NearPlane, FarPlane);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += Up * velocity;
    if (direction == DOWN)
        Position -= Up * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // Recalculate Right and Up vectors
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::LookAt(const glm::vec3 &targetPosition)
{
    glm::vec3 direction = glm::normalize(targetPosition - Position);

    Pitch = glm::degrees(asin(direction.y));
    Yaw = glm::degrees(atan2(direction.z, direction.x));

    updateCameraVectors();
}

void Camera::OnMouseButtonDown(int x, int y)
{
    if (!mouseButtonDown)
    {
        mouseButtonDown = true;
        lastMouseX = x;
        lastMouseY = y;
        firstMouse = true;
    }
}

void Camera::OnMouseButtonUp()
{
    mouseButtonDown = false;
}

void Camera::OnMouseMove(int x, int y)
{
    if (mouseButtonDown)
    {
        if (firstMouse)
        {
            lastMouseX = x;
            lastMouseY = y;
            firstMouse = false;
        }

        float xoffset = x - lastMouseX;
        float yoffset = lastMouseY - y; // Reversed since y-coordinates go from bottom to top

        lastMouseX = x;
        lastMouseY = y;

        ProcessMouseMovement(xoffset, yoffset);
    }
}

void Camera::ProcessKeyboardInput(const Uint8 *state, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (state[SDL_SCANCODE_W])
        Position += Front * velocity;
    if (state[SDL_SCANCODE_S])
        Position -= Front * velocity;
    if (state[SDL_SCANCODE_A])
        Position -= Right * velocity;
    if (state[SDL_SCANCODE_D])
        Position += Right * velocity;
    if (state[SDL_SCANCODE_SPACE])
        Position += Up * velocity;
    if (state[SDL_SCANCODE_X])
        Position -= Up * velocity;
}
