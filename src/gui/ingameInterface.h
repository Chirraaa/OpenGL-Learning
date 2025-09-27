#include <glad/glad.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

// Forward declarations to avoid circular dependencies
class Camera;
class Player;
class World;
enum class Face;

struct RaycastInfo {
	bool blockSelected;
	glm::ivec3 selectedBlockPos;
	Face selectedBlockFace;
};

class IngameInterface {
public:
	IngameInterface(GLFWwindow* window);
	~IngameInterface() = default;

	void initImGui();
	void cleanupImGui();
	void updateFPS(double currentTime);
	void renderImGui(const World& world, Player& player, const Camera& cam,
		float deltaTime, const RaycastInfo& raycastInfo, bool guiMode = false);

private:
	GLFWwindow* window;
	float fps;
	float frameTime;
	int frameCount;
	double lastFPSTime;
};
