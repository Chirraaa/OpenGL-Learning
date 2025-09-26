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

// std::vector<Sphere> launchObjects;

SphereArray launchObjects;

Voxel testBlock;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(double dt);

void setupSelectionOutline();
void renderSelectionOutline(const glm::mat4& view, const glm::mat4& projection);
void setupCrosshair();
void renderCrosshair();
void performRaycasting();
void processInput(double dt);

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

	while (!screen.shouldClose()) {
		double currentTime = glfwGetTime();
		deltaTime = currentTime - lastFrame;
		lastFrame = currentTime;

		processInput(deltaTime);

		performRaycasting();


		screen.update();

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

		for (int i = 0; i < launchObjects.instances.size(); i++) {
			if (glm::length(currentCam->cameraPos - launchObjects.instances[i].pos.y) > 50.0f) {
				removeObjects.push(i);
				continue;
			}
		}

		for (int i = removeObjects.size() - 1; i >= 0; i--) {
			launchObjects.instances.erase(launchObjects.instances.begin() + removeObjects.top());
			removeObjects.pop();
		}

		if (launchObjects.instances.size() > 0) {
			launchObjects.render(shader, deltaTime);
		}
		//for (Sphere& s : launchObjects) {
		//	s.render(shader, deltaTime);
		//}

		//for(int i = 0; i < 10; i++) 
		//	cubes[i].render(shader);

		//sphere.render(shader, deltaTime);

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

		screen.newFrame();
	}

	//for (auto& c : chunks)
	//	c.cleanup();


	

	//testBlock.cleanup();

	launchObjects.cleanup();

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

	RigidBody rb(0.25f, currentCam->cameraPos + glm::vec3(3.0f, 0.0f, 0.0f));
	rb.mass = 0.2f;
	rb.applyImpulse(currentCam->cameraFront, 1350.0f, dt);
	rb.applyAcceleration(Environment::gravitationalAcceleration);
	launchObjects.instances.push_back(rb);

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

	// change camera
	if (Keyboard::keyWentDown(GLFW_KEY_C)) {
		if (currentCam == &cameras[0]) {
			currentCam = &cameras[1];
		}
		else {
			currentCam = &cameras[0];
		}
	}

	// Use a vector to accumulate movement directions
	glm::vec3 moveDirection(0.0f);

	// Get camera's forward and right vectors, but keep them horizontal (ignore Y component)
	glm::vec3 forward = glm::normalize(glm::vec3(currentCam->cameraFront.x, 0.0f, currentCam->cameraFront.z));
	glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

	// Build movement direction based on input
	if (Keyboard::key(GLFW_KEY_W)) {
		moveDirection += forward;
	}
	if (Keyboard::key(GLFW_KEY_S)) {
		moveDirection -= forward;
	}
	if (Keyboard::key(GLFW_KEY_D)) {
		moveDirection += right;  // Fixed: was negative
	}
	if (Keyboard::key(GLFW_KEY_A)) {
		moveDirection -= right;  // Fixed: was positive
	}

	// Jump
	if (Keyboard::key(GLFW_KEY_SPACE)) {
		player.jump();
	}

	// Debug: Teleport to safe position if stuck (T key)
	if (Keyboard::keyWentDown(GLFW_KEY_T)) {
		player.teleportToSafePosition();
	}

	// Creative mode flying down (optional)
	if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
		// For now, just regular movement - you can implement creative flying later
		// currentCam->updateCameraPos(CameraDirection::DOWN, dt);
	}

	// Apply movement to player
	player.move(moveDirection);

	// Handle mouse look
	double dx = Mouse::getDX();
	double dy = Mouse::getDY();

	if (dx != 0 || dy != 0) {
		currentCam->updateCameraDirection(dx, dy);
	}

	// Handle zoom
	double scrollDy = Mouse::getScrollDY();
	if (scrollDy != 0) {
		currentCam->updateCameraZoom(scrollDy);
	}

	// Launch projectiles
	if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_LEFT)) {
		launchItem(dt);
	}

	if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_RIGHT)) {
		if (blockSelected) {
			world.setBlock(selectedBlockPos.x, selectedBlockPos.y, selectedBlockPos.z, VoxelType::AIR);
			blockSelected = false; // Deselect after breaking
		}
	}
}

// helper
glm::ivec3 worldToBlockCoords(glm::vec3 worldPos) {
	return glm::ivec3(
		static_cast<int>(std::floor(worldPos.x)),
		static_cast<int>(std::floor(worldPos.y)),
		static_cast<int>(std::floor(worldPos.z))
	);
}

void performRaycasting() {
	const float maxDist = 6.0f;
	const float stepSize = 0.05f;

	glm::vec3 rayStart = currentCam->cameraPos;
	glm::vec3 rayDir = glm::normalize(currentCam->cameraFront);

	blockSelected = false;

	for (float distance = 0.0f; distance < maxDist; distance += stepSize) {
		glm::vec3 currentPos = rayStart + rayDir * distance;

		// Block coordinates (blocks occupy integer coordinate ranges)
		glm::ivec3 blockPos = glm::ivec3(
			static_cast<int>(std::floor(currentPos.x)),
			static_cast<int>(std::floor(currentPos.y)),
			static_cast<int>(std::floor(currentPos.z))
		);

		VoxelType type = world.getBlockTypeAt(blockPos.x, blockPos.y, blockPos.z);

		if (type != VoxelType::AIR) {
			blockSelected = true;
			selectedBlockPos = blockPos;
			return;
		}
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