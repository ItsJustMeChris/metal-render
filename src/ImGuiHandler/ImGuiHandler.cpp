#include "ImGuiHandler.hpp"
#include "Engine.hpp"
#include "Camera.hpp"
#include "Renderable.hpp"
#include <string>

ImGuiHandler::ImGuiHandler(SDL_Window *window, MTL::Device *device)
    : window(window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(device);
    ImGui_ImplSDL2_InitForMetal(window);
}

ImGuiHandler::~ImGuiHandler()
{
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiHandler::processEvent(SDL_Event *event)
{
    ImGui_ImplSDL2_ProcessEvent(event);
}

void ImGuiHandler::render(MTL::CommandBuffer *commandBuffer, MTL::RenderPassDescriptor *renderPassDescriptor, uint32_t screenWidth, uint32_t screenHeight)
{
    // Pass the correct renderPassDescriptor to ImGui
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    drawInterface();

    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();

    // Create a new render command encoder for ImGui
    MTL::RenderCommandEncoder *imguiRenderCommandEncoder =
        commandBuffer->renderCommandEncoder(renderPassDescriptor);

    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, imguiRenderCommandEncoder);

    imguiRenderCommandEncoder->endEncoding();
    imguiRenderCommandEncoder->release();
}

void ImGuiHandler::drawInterface()
{
    // Obtain references to required engine components
    auto &renderables = engine->getRenderer()->getRenderables();
    auto *camera = engine->getCamera();
    auto aspectRatio = engine->getRenderer()->aspectRatio();

    ImGuiIO &io = ImGui::GetIO();
    float dpiScaleFactor = engine->getRenderer()->dimensions().x / io.DisplaySize.x;

    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    ImGui::Begin("Renderables");

    int index = 0;

    for (auto &renderable : renderables)
    {
        std::string renderableName = "Renderable " + std::to_string(index);

        if (ImGui::CollapsingHeader(renderableName.c_str()))
        {
            glm::vec4 viewport = engine->getRenderer()->viewport();

            glm::vec2 screenPos = camera->WorldToScreen(
                renderable->getPosition(),
                camera->GetProjectionMatrix(aspectRatio),
                camera->GetViewMatrix(),
                viewport);

            bool isInFrontOfCamera =
                (screenPos.x != -FLT_MAX && screenPos.y != -FLT_MAX);

            screenPos /= dpiScaleFactor;

            if (isInFrontOfCamera)
            {
                drawList->AddLine(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2),
                                  ImVec2(screenPos.x, screenPos.y),
                                  IM_COL32(255, 255, 255, 255), 2.0f);

                drawList->AddText(ImVec2(screenPos.x, screenPos.y),
                                  IM_COL32(255, 255, 255, 255),
                                  renderableName.c_str());
            }

            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        renderable->getPosition().x,
                        renderable->getPosition().y,
                        renderable->getPosition().z);
            ImGui::Text("Status: %s",
                        isInFrontOfCamera ? "In front of the camera"
                                          : "Behind the camera or invalid");

            if (ImGui::Button(("Teleport##" + std::to_string(index)).c_str()))
            {
                camera->Teleport(renderable->getPosition());
            }
            ImGui::SameLine();
            if (ImGui::Button(("Look At##" + std::to_string(index)).c_str()))
            {
                camera->LookAt(renderable->getPosition());
            }
        }
        index++;
    }

    ImGui::End();

    ImGui::Begin("Camera Controls");

    ImGui::Text("Screen Size: (%f, %f)", engine->getRenderer()->dimensions().x, engine->getRenderer()->dimensions().y);
    ImGui::Text("ImGui Display Size: (%.0f, %.0f)",
                io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);

    static float teleportCoords[3] = {490.0f, -281.0f, -4387.0f};
    ImGui::InputFloat3("Teleport Coordinates", teleportCoords);

    if (ImGui::Button("Teleport##Camera"))
    {
        camera->Teleport(glm::vec3(teleportCoords[0], teleportCoords[1], teleportCoords[2]));
    }

    ImGui::End();
}

bool ImGuiHandler::wantsMouseCapture() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiHandler::wantsKeyboardCapture() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}
