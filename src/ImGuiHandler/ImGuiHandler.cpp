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

void ImGuiHandler::render(MTL::CommandBuffer *commandBuffer, MTL::RenderPassDescriptor *renderPassDescriptor)
{
    // Pass the correct renderPassDescriptor to ImGui
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    drawInterface();

    ImGui::Render();
    ImDrawData *drawData = ImGui::GetDrawData();

    // Create a new render command encoder for ImGui
    MTL::RenderCommandEncoder *imguiRenderCommandEncoder = commandBuffer->renderCommandEncoder(renderPassDescriptor);

    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, imguiRenderCommandEncoder);

    imguiRenderCommandEncoder->endEncoding();
    imguiRenderCommandEncoder->release();
}

glm::vec3 Start = {0.0f, 0.0f, 0.0f};
glm::vec3 End = {0.0f, 0.0f, 0.0f};

glm::vec3 intersection = {0.0f, 0.0f, 0.0f};

void ImGuiHandler::drawInterface()
{
    // Obtain references to required engine components
    auto &renderables = engine->getRenderer()->getRenderables();
    auto *camera = engine->getCamera();
    auto aspectRatio = engine->getRenderer()->aspectRatio();

    ImGuiIO &io = ImGui::GetIO();
    float dpiScaleFactor = engine->getRenderer()->dimensions().x / io.DisplaySize.x;

    ImDrawList *drawList = ImGui::GetBackgroundDrawList();

    {
        // Cross Hair
        ImVec2 center = ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2);
        drawList->AddLine(ImVec2(center.x - 10, center.y), ImVec2(center.x + 10, center.y), IM_COL32(255, 255, 255, 255), 2.0f);
        drawList->AddLine(ImVec2(center.x, center.y - 10), ImVec2(center.x, center.y + 10), IM_COL32(255, 255, 255, 255), 2.0f);
    }

    {
        ImGui::Begin("Renderables");

        int index = 0;

        for (auto &renderable : renderables)
        {
            std::string renderableName = renderable->name + " (" + std::to_string(index) + ")";

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

                glm::vec3 position = renderable->getPosition();
                float pos[3] = {position.x, position.y, position.z};
                if (ImGui::DragFloat3(("Position##" + std::to_string(index)).c_str(), pos, 0.1f))
                {
                    renderable->setPosition(glm::vec3(pos[0], pos[1], pos[2]));
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

    {
        ImGui::Begin("Lighting Controls");

        ImGui::Text("Ambient Light Color");
        LightData &data = engine->getRenderer()->lightData;
        ImGui::ColorEdit3("Ambient Light", (float *)&data.ambientColor);

        ImGui::Text("Light Color");
        ImGui::ColorEdit3("Color", (float *)&data.lightColor);

        ImGui::End();
    }

    {
        ImGui::Begin("Ray Tracing");

        Start = camera->GetPosition();
        End = camera->GetPosition() + camera->GetFront() * 100.0f;

        ImGui::Text("Start: (%.2f, %.2f, %.2f)", Start.x, Start.y, Start.z);

        intersection = engine->getRenderer()->TraceLine(Start, End);

        ImGui::Text("Intersection: (%.2f, %.2f, %.2f)", intersection.x, intersection.y, intersection.z);

        if (intersection.x != 0)
        {
            glm::vec4 viewport = engine->getRenderer()->viewport();
            glm::vec2 screenPosStart = camera->WorldToScreen(
                Start,
                camera->GetProjectionMatrix(aspectRatio),
                camera->GetViewMatrix(),
                viewport);

            glm::vec2 screenPosEnd = camera->WorldToScreen(
                End,
                camera->GetProjectionMatrix(aspectRatio),
                camera->GetViewMatrix(),
                viewport);

            glm::vec2 screenPosIntersection = camera->WorldToScreen(
                intersection,
                camera->GetProjectionMatrix(aspectRatio),
                camera->GetViewMatrix(),
                viewport);

            screenPosStart /= dpiScaleFactor;
            screenPosEnd /= dpiScaleFactor;
            screenPosIntersection /= dpiScaleFactor;

            // drawList->AddLine(ImVec2(screenPosStart.x, screenPosStart.y), ImVec2(screenPosIntersection.x, screenPosIntersection.y), IM_COL32(0, 255, 0, 255), 2.0f);
            // drawList->AddLine(ImVec2(screenPosIntersection.x, screenPosIntersection.y), ImVec2(screenPosEnd.x, screenPosEnd.y), IM_COL32(255, 0, 0, 255), 2.0f);

            drawList->AddText(ImVec2(screenPosIntersection.x, screenPosIntersection.y),
                              IM_COL32(255, 255, 255, 255),
                              "Intersection");
        }

        ImGui::End();
    }
}

bool ImGuiHandler::wantsMouseCapture() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiHandler::wantsKeyboardCapture() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}
