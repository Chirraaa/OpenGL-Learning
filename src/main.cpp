#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>

#include <fstream>
#include <sstream>
#include <streambuf>
#include <iostream>
#include <vector>
#include <stack>

#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "graphics/Model.h"
#include "graphics/Light.h"


#include "graphics/models/cube.hpp"
#include "graphics/models/lamp.hpp"
#include "graphics/models/gun.hpp"
#include "graphics/models/sphere.hpp"
#include "graphics/models/voxel.hpp"

#include "io/Keyboard.h"
#include "io/Mouse.h"
#include "io/Camera.h"
#include "io/Screen.h"
#include "physics/Environment.h"
#include "graphics/models/voxelchunk.hpp"

#include "graphics/env/World.h"

#include "player/Player.h"

#include "gui/ingameInterface.h"
#include "graphics/models/donut.hpp"


struct RaycastHit {
	bool hit = false;
	glm::ivec3 blockPos;
	glm::vec3 hitPoint;
	Face hitFace;
	glm::vec3 faceNormal;
};

Face selectedBlockFace;

glm::mat4 transform = glm::mat4(1.0f);

//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));


World world(4, 9429238);

Camera cameras[2] = {
	Camera(glm::vec3(0.0f, 90.0f, 0.0f)), // Start higher up to see terrain
	Camera(glm::vec3(0.0f, 90.0f, 0.0f))
};



Shader selectionShader;
unsigned int selectionVAO, selectionVBO;
bool blockSelected = false;
glm::ivec3 selectedBlockPos;

Shader crosshairShader;
unsigned int crosshairVAO, crosshairVBO;



double deltaTime = 0.0f;
double lastFrame = 0.0f;

unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

Screen screen;


//VoxelChunk chunk(glm::vec3(-8, -32, -8));

std::vector<VoxelChunk> chunks;



Player player(glm::vec3(0.0f, 70.0f, 0.0f), &cameras[0], &world);

Camera* currentCam = player.getCamera();

float x, y, z;

//Sphere sphere(glm::vec3(0.0f), glm::vec3(1.0f));

//Donut d(glm::vec3(10.0f, 35.0f, 0.0f), glm::vec3(0.05f, 0.025f, 0.025f));

// std::vector<Sphere> launchObjects;

DonutArray launchDonuts;

SphereArray launchObjects;

Voxel testBlock;

// GUI
IngameInterface* ui = nullptr;
bool guiMode = false;


// function declaration
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(double dt);

void setupSelectionOutline();
void renderSelectionOutline(const glm::mat4& view, const glm::mat4& projection);
void setupCrosshair();
void renderCrosshair();
void performRaycasting();
void processInput(double dt);
RaycastHit performRaycastingWithFace();
Face calculateHitFace(glm::vec3 hitPoint, glm::ivec3 blockPos);
glm::vec3 getFaceNormal(Face face);
glm::ivec3 calculateNewBlockPosition(glm::ivec3 hitBlockPos, Face hitFace);
bool isValidPlacementPosition(glm::ivec3 pos);
void toggleGUIMode();


int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (!screen.init()) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to init GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	screen.setParameters();

	glEnable(GL_DEPTH_TEST);

	// Shaders

	Shader shader("assets/vertex_core.glsl", "assets/fragment_core.glsl");
	Shader lampShader("assets/vertex_core.glsl", "assets/lamp.fs");

	selectionShader = Shader("assets/selection.vs", "assets/selection.fs");
	crosshairShader = Shader("assets/crosshair.vs", "assets/crosshair.fs");

	// SETUP RENDERING OBJECTS
	setupSelectionOutline();
	setupCrosshair();

	// Models
	//Model m(glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.05f), true);
	//m.loadModel("assets/models/lowpoly Rifle/scene.gltf");

	//Gun g;
	//g.loadModel("assets/models/lowpoly Rifle/scene.gltf");
	


	//d.init();

	//sphere.init();

	//chunk.generateTerrain();
	//chunk.generateSimpleHills();
	//chunk.generateMountains();
	//chunk.generateTerrain();

	//testBlock.init("grass_block_top.png");

	//for (int i = 0; i < 4; i++) {
	//	for(int j = 0; j < 4; j++) {
	//		chunks.push_back(VoxelChunk(glm::vec3(-8 + i * 16, -32, -8 + j * 16), 2362436 + i * 10 + j * 100));
	//		chunks.back().generateTerrain();
	//	}
	//}


	launchObjects.init();
	launchDonuts.init();
	launchDonuts.setWorld(&world);

	glm::vec3 cubePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f,  2.0f, -2.5f),
		glm::vec3(1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	};

	//Cube cubes[10];
	//for (unsigned int i = 0; i < 10; i++) {
	//	cubes[i] = Cube(Material::gold, cubePositions[i], glm::vec3(1.0f));
	//	cubes[i].init();
	//}


	DirLight dirLight = { glm::vec3(0.0f, -1.0f, -0.5f),
		glm::vec4(0.1f,0.1f,0.1f,0.1f),
		glm::vec4(1.0f,1.0f,1.0f,1.0f),
		glm::vec4(0.75f,0.75f,0.75f,1.0f) };


	glm::vec3 pointLightPositions[] = {
			glm::vec3(0.7f,  0.2f,  2.0f),
			glm::vec3(2.3f, -3.3f, -4.0f),
			glm::vec3(-4.0f,  2.0f, -12.0f),
			glm::vec3(0.0f,  0.0f, -3.0f)
	};

	glm::vec4 ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
	glm::vec4 diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	glm::vec4 specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	float k0 = 1.0f;
	float k1 = 0.09f;
	float k2 = 0.032f;

	//Lamp lamps[4];
	//for (unsigned int i = 0; i < 4; i++) {
	//	lamps[i] = Lamp(glm::vec3(1.0f),
	//		ambient, diffuse, specular,
	//		k0, k1, k2,
	//		pointLightPositions[i], glm::vec3(0.25f));
	//	lamps[i].init();
	//}

	//LampArray lamps;
	//lamps.init();
	//for (unsigned int i = 0; i < 4; i++) {
	//	lamps.lightInstances.push_back({ pointLightPositions[i],
	//		k0, k1, k2,
	//		ambient, diffuse, specular
	//		});
	//}


	SpotLight spotLight = {
		currentCam->cameraPos,
		currentCam->cameraFront,
		glm::cos(glm::radians(12.5f)),
		glm::cos(glm::radians(15.0f)),
		1.0f, 0.09f, 0.032f,
		glm::vec4(0.0f,0.0f,0.0f,1.0f),
		glm::vec4(1.0f,1.0f,1.0f,1.0f),
		glm::vec4(1.0f,1.0f,1.0f,1.0f)
	};

	x = 0.0f;
	y = 0.0f;
	z = 3.0f;


	// GUI 

	ui = new IngameInterface(screen.getWindow());
	ui->initImGui();

	while (!screen.shouldClose()) {
		double currentTime = glfwGetTime();
		deltaTime = currentTime - lastFrame;
		lastFrame = currentTime;

		processInput(deltaTime);

		performRaycasting();


		screen.update();

		ui->updateFPS(currentTime);

		//player.update(deltaTime);

		shader.activate();
		shader.set3Float("viewPos", currentCam->cameraPos);


		dirLight.render(shader);

		//for (unsigned int i = 0; i < 4; i++) {
		//	lamps.lightInstances[i].render(shader, i);
		//}

		shader.setInt("noPointLights", 0);

		spotLight.position = currentCam->cameraPos;
		spotLight.direction = currentCam->cameraFront;
		spotLight.render(shader, 0);
		shader.setInt("noSpotLights", 0);

		// create transformation for screen

		//m.angle = (float)glfwGetTime() * 50.0f;
		//m.rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);


		view = currentCam->getViewMatrix();
		//view = glm::translate(view, glm::vec3(-x, -y, -z));
		projection = glm::perspective(glm::radians(currentCam->zoom), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);


		shader.setMat4("model", model);
		shader.setMat4("view", view);
		shader.setMat4("projection", projection);

		std::stack<int> removeObjects;

		for (int i = 0; i < launchDonuts.instances.size(); i++) {
			if (glm::length(currentCam->cameraPos - launchDonuts.instances[i].pos.y) > 50.0f) {
				removeObjects.push(i);
				continue;
			}
		}

		for (int i = removeObjects.size() - 1; i >= 0; i--) {
			launchDonuts.instances.erase(launchDonuts.instances.begin() + removeObjects.top());
			removeObjects.pop();
		}

		if (launchDonuts.instances.size() > 0) {
			launchDonuts.render(shader, deltaTime);
		}

		//for (Donut& d : launchDonuts) {
		//	d.render(shader, deltaTime);
		//}

		//for(int i = 0; i < 10; i++) 
		//	cubes[i].render(shader);

		//sphere.render(shader, deltaTime);

		// Donut activities
		//d.render(shader, deltaTime);

		//chunk.render(shader);

		//for (auto& c : chunks)
		//	c.render(shader);

		player.update(deltaTime);

		static bool firstFrame = true;
		if (firstFrame) {
			std::cout << "Camera starting position: ("
				<< currentCam->cameraPos.x << ", "
				<< currentCam->cameraPos.y << ", "
				<< currentCam->cameraPos.z << ")" << std::endl;
			firstFrame = false;
		}

		world.update(currentCam->cameraPos); // Update world based on camera position

		// Also add this right before world.render(shader):
		static int frameCount = 0;
		frameCount++;
		if (frameCount % 60 == 0) { // Every 60 frames
			std::cout << "Frame " << frameCount << " - Camera at: ("
				<< currentCam->cameraPos.x << ", "
				<< currentCam->cameraPos.y << ", "
				<< currentCam->cameraPos.z << ")" << std::endl;
		}

		world.render(shader);

		if (blockSelected) {
			renderSelectionOutline(view, projection);
		}

		//lampShader.activate();
		//lampShader.setMat4("view", view);
		//lampShader.setMat4("projection", projection);

		//lamps.render(lampShader, deltaTime);

		renderCrosshair();

		RaycastInfo raycastInfo = { blockSelected, selectedBlockPos, selectedBlockFace };
		ui->renderImGui(world, player, *currentCam, deltaTime, raycastInfo, guiMode);





		screen.newFrame();
	}
	//for (auto& c : chunks)
	//	c.cleanup();




	//testBlock.cleanup();

	ui->cleanupImGui();
	delete ui;

	launchDonuts.cleanup();

	//for (int i = 0; i < 10; i++)
	//	cubes[i].cleanup();

	//sphere.cleanup();
	//dirtBlock.cleanup();
	//for (int i = 0; i < 4; i++) {
	//	for (int j = 0; j < planeBlocks[i].size(); j++) {
	//		planeBlocks[i][j].cleanup();
	//	}
	//}

	//chunk.cleanup();

	//lamps.cleanup();
	glDeleteVertexArrays(1, &selectionVAO);
	glDeleteBuffers(1, &selectionVBO);
	glDeleteVertexArrays(1, &crosshairVAO);
	glDeleteBuffers(1, &crosshairVBO);

	world.cleanup();
	glfwTerminate();
	return 0;
}



// FUNCTIONS



void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
}

void launchItem(float dt) {

	RigidBody rb(1.0f, currentCam->cameraPos + glm::vec3(3.0f, 0.0f, 0.0f));
	rb.mass = 0.2f;
	rb.applyImpulse(currentCam->cameraFront, 500.0f, dt);
	rb.applyAcceleration(Environment::gravitationalAcceleration);
	launchDonuts.instances.push_back(rb);

	//Sphere newSphere(currentCam->cameraPos, glm::vec3(0.25f));
	//newSphere.init();
	//newSphere.rb.mass = 0.2f;
	//newSphere.rb.applyAcceleration(Environment::gravitationalAcceleration);
	//newSphere.rb.applyImpulse(currentCam->cameraFront, 1350.0f, dt);
	//launchObjects.push_back(newSphere);
}

void processInput(double dt) {
	if (Keyboard::key(GLFW_KEY_ESCAPE)) {
		screen.setShouldClose(true);
	}

	// Toggle GUI mode with Tab key
	if (Keyboard::keyWentDown(GLFW_KEY_TAB)) {
		guiMode = !guiMode;
		std::cout << "GUI Mode: " << (guiMode ? "ON" : "OFF") << std::endl;

		if (guiMode) {
			glfwSetInputMode(screen.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			glfwSetInputMode(screen.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		return; // Don't process other input this frame
	}

	// Get mouse input BUT only use it for camera if not in GUI mode
	double dx = Mouse::getDX();
	double dy = Mouse::getDY();

	// Only update camera if NOT in GUI mode
	if (!guiMode && (dx != 0 || dy != 0)) {
		currentCam->updateCameraDirection(dx, dy);
	}

	// Handle zoom (only in game mode)
	if (!guiMode) {
		double scrollDy = Mouse::getScrollDY();
		if (scrollDy != 0) {
			currentCam->updateCameraZoom(scrollDy);
		}
	}

	// Only process keyboard/mouse clicks if not in GUI mode
	if (!guiMode) {
		// change camera
		if (Keyboard::keyWentDown(GLFW_KEY_C)) {
			if (currentCam == &cameras[0]) {
				currentCam = &cameras[1];
			}
			else {
				currentCam = &cameras[0];
			}
		}

		glm::vec3 moveDirection(0.0f);
		glm::vec3 forward = glm::normalize(glm::vec3(currentCam->cameraFront.x, 0.0f, currentCam->cameraFront.z));
		glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

		if (Keyboard::key(GLFW_KEY_W)) {
			moveDirection += forward;
		}
		if (Keyboard::key(GLFW_KEY_S)) {
			moveDirection -= forward;
		}
		if (Keyboard::key(GLFW_KEY_D)) {
			moveDirection += right;
		}
		if (Keyboard::key(GLFW_KEY_A)) {
			moveDirection -= right;
		}

		if (Keyboard::key(GLFW_KEY_SPACE)) {
			player.jump();
		}

		if (Keyboard::keyWentDown(GLFW_KEY_L)) {
			launchItem(deltaTime);
		}

		if (Keyboard::key(GLFW_KEY_LEFT_SHIFT) && !player.isGravityEnabled()) {
			player.moveVertical(-1.0f); // Fly down
		}

		if (Keyboard::keyWentDown(GLFW_KEY_T)) {
			player.teleportToSafePosition();
		}

		player.move(moveDirection);

		if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_LEFT)) {
			if (blockSelected) {
				world.setBlock(selectedBlockPos.x, selectedBlockPos.y, selectedBlockPos.z, VoxelType::AIR);
				blockSelected = false;
			}
		}

		if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_RIGHT)) {
			if (blockSelected) {
				glm::ivec3 newBlockPos = calculateNewBlockPosition(selectedBlockPos, selectedBlockFace);
				if (isValidPlacementPosition(newBlockPos)) {
					world.placeBlock(newBlockPos.x, newBlockPos.y, newBlockPos.z, VoxelType::COBBLESTONE);
				}
				else {
					std::cout << "Cannot place block there!" << std::endl;
				}
			}
		}
	}
}

void toggleGUIMode() {
	guiMode = !guiMode;

	if (guiMode) {
		// Enable GUI mode - show cursor
		glfwSetInputMode(screen.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		std::cout << "GUI Mode: ON" << std::endl;
	}
	else {
		// Enable game mode - hide cursor
		glfwSetInputMode(screen.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		std::cout << "GUI Mode: OFF" << std::endl;

		// Reset mouse to prevent camera jump
		Mouse::firstMouse = true;
	}
}

// helper functions

glm::ivec3 worldToBlockCoords(glm::vec3 worldPos) {
	return glm::ivec3(
		static_cast<int>(std::floor(worldPos.x)),
		static_cast<int>(std::floor(worldPos.y)),
		static_cast<int>(std::floor(worldPos.z))
	);
}

bool isValidPlacementPosition(glm::ivec3 pos) {
	// Don't place blocks inside the player
	glm::vec3 playerPos = player.getPosition();

	// Simple check - make sure it's not too close to player
	float distance = glm::length(glm::vec3(pos) - playerPos);
	if (distance < 1.5f) {
		return false;
	}

	// Make sure there's no block already there
	VoxelType existingType = world.getBlockTypeAt(pos.x, pos.y, pos.z);
	return existingType == VoxelType::AIR;
}

RaycastHit performRaycastingWithFace() {
	const float maxDist = 6.0f;
	const float stepSize = 0.02f; // Smaller step for better accuracy

	glm::vec3 rayStart = currentCam->cameraPos;
	glm::vec3 rayDir = glm::normalize(currentCam->cameraFront);

	RaycastHit result;

	for (float distance = 0.0f; distance < maxDist; distance += stepSize) {
		glm::vec3 currentPos = rayStart + rayDir * distance;

		glm::ivec3 blockPos = glm::ivec3(
			static_cast<int>(std::floor(currentPos.x)),
			static_cast<int>(std::floor(currentPos.y)),
			static_cast<int>(std::floor(currentPos.z))
		);

		VoxelType type = world.getBlockTypeAt(blockPos.x, blockPos.y, blockPos.z);

		if (type != VoxelType::AIR) {
			result.hit = true;
			result.blockPos = blockPos;
			result.hitPoint = currentPos;

			// Calculate which face was hit
			result.hitFace = calculateHitFace(currentPos, blockPos);
			result.faceNormal = getFaceNormal(result.hitFace);

			return result;
		}
	}

	return result; // No hit
}

// Function to determine which face of the block was hit
Face calculateHitFace(glm::vec3 hitPoint, glm::ivec3 blockPos) {
	// Convert block coordinates to the actual block boundaries
	// Blocks occupy space from blockPos to blockPos + 1
	float blockMinX = blockPos.x;
	float blockMaxX = blockPos.x + 1.0f;
	float blockMinY = blockPos.y;
	float blockMaxY = blockPos.y + 1.0f;
	float blockMinZ = blockPos.z;
	float blockMaxZ = blockPos.z + 1.0f;

	// Calculate distance to each face
	float distToLeft = std::abs(hitPoint.x - blockMinX);  // Left face (x = blockMinX)
	float distToRight = std::abs(hitPoint.x - blockMaxX);  // Right face (x = blockMaxX)
	float distToBottom = std::abs(hitPoint.y - blockMinY);  // Bottom face (y = blockMinY)
	float distToTop = std::abs(hitPoint.y - blockMaxY);  // Top face (y = blockMaxY)
	float distToBack = std::abs(hitPoint.z - blockMinZ);  // Back face (z = blockMinZ)
	float distToFront = std::abs(hitPoint.z - blockMaxZ);  // Front face (z = blockMaxZ)

	// Find the minimum distance (closest face)
	float minDist = std::min({ distToLeft, distToRight, distToBottom, distToTop, distToBack, distToFront });

	if (minDist == distToLeft) return Face::LEFT;
	if (minDist == distToRight) return Face::RIGHT;
	if (minDist == distToBottom) return Face::BOTTOM;
	if (minDist == distToTop) return Face::TOP;
	if (minDist == distToBack) return Face::BACK;
	return Face::FRONT;
}

// Get the normal vector for a face
glm::vec3 getFaceNormal(Face face) {
	switch (face) {
	case Face::FRONT:  return glm::vec3(0, 0, 1);
	case Face::BACK:   return glm::vec3(0, 0, -1);
	case Face::LEFT:   return glm::vec3(-1, 0, 0);
	case Face::RIGHT:  return glm::vec3(1, 0, 0);
	case Face::TOP:    return glm::vec3(0, 1, 0);
	case Face::BOTTOM: return glm::vec3(0, -1, 0);
	}
	return glm::vec3(0, 0, 0);
}

// Calculate where to place the new block
glm::ivec3 calculateNewBlockPosition(glm::ivec3 hitBlockPos, Face hitFace) {
	glm::vec3 normal = getFaceNormal(hitFace);
	return hitBlockPos + glm::ivec3(normal);
}

// Updated main raycasting function for your existing code
void performRaycasting() {
	RaycastHit hit = performRaycastingWithFace();

	if (hit.hit) {
		blockSelected = true;
		selectedBlockPos = hit.blockPos;

		// Store the hit face for block placement (you'll need to add this variable)
		selectedBlockFace = hit.hitFace;

		// Debug output
		std::cout << "Hit block at: (" << hit.blockPos.x << ", " << hit.blockPos.y << ", " << hit.blockPos.z << ")" << std::endl;
		std::cout << "Hit face: " << static_cast<int>(hit.hitFace) << std::endl;
		std::cout << "Hit point: (" << hit.hitPoint.x << ", " << hit.hitPoint.y << ", " << hit.hitPoint.z << ")" << std::endl;

		glm::ivec3 newBlockPos = calculateNewBlockPosition(hit.blockPos, hit.hitFace);
		std::cout << "New block would go at: (" << newBlockPos.x << ", " << newBlockPos.y << ", " << newBlockPos.z << ")" << std::endl;
	}
	else {
		blockSelected = false;
	}
}

void setupSelectionOutline() {
	// UPDATED vertices to be centered around (0,0,0)
	// The small offset (0.505) makes the box slightly larger to avoid graphical glitches.
	float s = 0.505f;
	float vertices[] = {
		// 4 lines along X axis
		-s, -s, -s,   s, -s, -s,
		-s,  s, -s,   s,  s, -s,
		-s, -s,  s,   s, -s,  s,
		-s,  s,  s,   s,  s,  s,
		// 4 lines along Y axis
		-s, -s, -s,  -s,  s, -s,
		 s, -s, -s,   s,  s, -s,
		-s, -s,  s,  -s,  s,  s,
		 s, -s,  s,   s,  s,  s,
		 // 4 lines along Z axis
		 -s, -s, -s,  -s, -s,  s,
		  s, -s, -s,   s, -s,  s,
		 -s,  s, -s,  -s,  s,  s,
		  s,  s, -s,   s,  s,  s
	};
	glGenVertexArrays(1, &selectionVAO);
	glGenBuffers(1, &selectionVBO);
	glBindVertexArray(selectionVAO);
	glBindBuffer(GL_ARRAY_BUFFER, selectionVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void renderSelectionOutline(const glm::mat4& view, const glm::mat4& projection) {
	selectionShader.activate();

	glm::vec3 outlinePos = glm::vec3(selectedBlockPos) + glm::vec3(0.5f, 0.5f, 0.5f);
	glm::mat4 model = glm::translate(glm::mat4(1.0f), outlinePos);

	selectionShader.setMat4("model", model);
	selectionShader.setMat4("view", view);
	selectionShader.setMat4("projection", projection);

	glLineWidth(2.0f);
	glBindVertexArray(selectionVAO);
	glDrawArrays(GL_LINES, 0, 24);
	glBindVertexArray(0);
}

void setupCrosshair() {
	float vertices[] = {
		// Horizontal line
		-0.008f,  0.0f,
		 0.008f,  0.0f,
		 // Vertical line
		  0.0f, -0.01f,
		  0.0f,  0.01f
	};
	glGenVertexArrays(1, &crosshairVAO);
	glGenBuffers(1, &crosshairVBO);
	glBindVertexArray(crosshairVAO);
	glBindBuffer(GL_ARRAY_BUFFER, crosshairVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void renderCrosshair() {
	glDisable(GL_DEPTH_TEST); // Draw on top of everything

	crosshairShader.activate();
	// Use an orthographic projection for 2D rendering
	glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);
	// Move the crosshair to the center
	projection = glm::translate(projection, glm::vec3(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0.0f));
	// Scale it up
	projection = glm::scale(projection, glm::vec3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f));

	crosshairShader.setMat4("projection", projection);

	glLineWidth(2.0f);
	glBindVertexArray(crosshairVAO);
	glDrawArrays(GL_LINES, 0, 4);
	glBindVertexArray(0);

	glEnable(GL_DEPTH_TEST); // Re-enable depth testing
}