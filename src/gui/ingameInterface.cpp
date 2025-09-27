#include "ingameInterface.h"
#include "../io/Camera.h"
#include "../player/Player.h"
#include "../graphics/env/World.h"
#include "../graphics/models/voxel.hpp"

IngameInterface::IngameInterface(GLFWwindow* window)
	: window(window),
	fps(0.0f),
	frameTime(0.0f),
	frameCount(0),
	lastFPSTime(0.0)
{
}

void IngameInterface::initImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void IngameInterface::cleanupImGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void IngameInterface::updateFPS(double currentTime) {
	frameCount++;
	if (currentTime - lastFPSTime >= 1.0) {
		fps = frameCount / (currentTime - lastFPSTime);
		frameTime = (currentTime - lastFPSTime) * 1000.0 / frameCount;
		frameCount = 0;
		lastFPSTime = currentTime;
	}
}

void IngameInterface::renderImGui(const World& world, Player& player,
	const Camera& cam, float deltaTime,
	const RaycastInfo& raycastInfo, bool guiMode) {

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Always show performance info, but make it less intrusive in game mode
	ImGuiWindowFlags windowFlags = guiMode ? 0 : ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse;

	// Performance window
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize | windowFlags);
	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("Frame Time: %.2f ms", frameTime);
	ImGui::Text("Delta Time: %.4f s", deltaTime);

	// Show GUI mode status
	ImGui::Separator();
	ImGui::Text("GUI Mode: %s", guiMode ? "ON (Tab to toggle)" : "OFF (Tab to toggle)");

	ImGui::Separator();

	// Camera info
	glm::vec3 camPos = cam.cameraPos;
	ImGui::Text("Camera Position:");
	ImGui::Text("  X: %.2f", camPos.x);
	ImGui::Text("  Y: %.2f", camPos.y);
	ImGui::Text("  Z: %.2f", camPos.z);
	ImGui::Separator();

	// Player info
	glm::vec3 playerPos = player.getPosition();
	ImGui::Text("Player Position:");
	ImGui::Text("  X: %.2f", playerPos.x);
	ImGui::Text("  Y: %.2f", playerPos.y);
	ImGui::Text("  Z: %.2f", playerPos.z);
	ImGui::Text("On Ground: %s", player.isOnGround() ? "Yes" : "No");

	// Selected block info
	if (raycastInfo.blockSelected) {
		ImGui::Separator();
		ImGui::Text("Selected Block:");
		ImGui::Text("  Position: (%d, %d, %d)",
			raycastInfo.selectedBlockPos.x,
			raycastInfo.selectedBlockPos.y,
			raycastInfo.selectedBlockPos.z);
		ImGui::Text("  Face: %d", static_cast<int>(raycastInfo.selectedBlockFace));
	}

	ImGui::End();

	// Only show interactive windows when in GUI mode
	if (guiMode) {
		// World info window
		ImGui::Begin("World Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Render Distance: %d", world.getRenderDistance());

		ImGui::Separator();
		ImGui::Text("Controls:");
		ImGui::Text("WASD - Move");
		ImGui::Text("Space - Jump");
		ImGui::Text("Mouse - Look");
		ImGui::Text("Left Click - Launch projectile");
		ImGui::Text("Right Click - Place block");
		ImGui::Text("T - Teleport to safe position");
		ImGui::Text("Tab - Toggle GUI Mode");

		ImGui::End();

		// Settings window
		static bool showSettings = true;
		if (ImGui::Begin("Settings", &showSettings)) {
			static int renderDistance = world.getRenderDistance();
			if (ImGui::SliderInt("Render Distance", &renderDistance, 1, 10)) {
				// You'll need to add a setter method to World class
				// world.setRenderDistance(renderDistance);
			}

			static bool wireframe = false;
			if (ImGui::Checkbox("Wireframe", &wireframe)) {
				if (wireframe) {
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}
				else {
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				}
			}

			bool gravityEnabled = player.isGravityEnabled();
			if (ImGui::Checkbox("Enable Gravity", &gravityEnabled)) {
				player.setGravityEnabled(gravityEnabled);
				std::cout << "Gravity toggled: " << (gravityEnabled ? "ON" : "OFF") << std::endl;
			}

		}
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}