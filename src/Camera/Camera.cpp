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

glm::vec2 Camera::WorldToScreen(const glm::vec3 &worldPosition, const glm::mat4 &projection, const glm::mat4 &view, const glm::vec4 &viewport) const
{
    glm::vec4 clipSpacePosition = projection * view * glm::vec4(worldPosition, 1.0f);

    // Check if the point is behind the camera
    if (clipSpacePosition.w <= 0.0f)
    {
        return glm::vec2(-FLT_MAX, -FLT_MAX); // Return an invalid position
    }

    glm::vec3 ndcSpacePosition = glm::vec3(clipSpacePosition) / clipSpacePosition.w;

    // Convert from NDC space to window space
    glm::vec2 windowSpacePosition;
    windowSpacePosition.x = viewport.x + viewport.z * (ndcSpacePosition.x + 1.0f) / 2.0f;
    windowSpacePosition.y = viewport.y + viewport.w * (1.0f - (ndcSpacePosition.y + 1.0f) / 2.0f); // Flip Y-coordinate

    return windowSpacePosition;
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
