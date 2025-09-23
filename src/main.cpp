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



glm::mat4 transform = glm::mat4(1.0f);

//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

Camera cameras[2] = {
	Camera(glm::vec3(0.0f, 0.0f, 0.0f)),
	Camera(glm::vec3(0.0f, 0.0f, 0.0f))
};

Camera* currentCam = &cameras[0];

double deltaTime = 0.0f;
double lastFrame = 0.0f;

unsigned int SCREEN_WIDTH = 800, SCREEN_HEIGHT = 600;

Screen screen;


VoxelChunk chunk(glm::vec3(-8, -32, -8));

float x, y, z;

//Sphere sphere(glm::vec3(0.0f), glm::vec3(1.0f));

// std::vector<Sphere> launchObjects;

SphereArray launchObjects;

Voxel testBlock;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
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


	// Models
	//Model m(glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.05f), true);
	//m.loadModel("assets/models/lowpoly Rifle/scene.gltf");

	//Gun g;
	//g.loadModel("assets/models/lowpoly Rifle/scene.gltf");

	//sphere.init();

	//chunk.generateTerrain();
	//chunk.generateSimpleHills();
	//chunk.generateMountains();
	chunk.generateTerrain();

	testBlock.init("grass_block_top.png");

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

		screen.update();

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

		chunk.render(shader);

		//lampShader.activate();
		//lampShader.setMat4("view", view);
		//lampShader.setMat4("projection", projection);

		//lamps.render(lampShader, deltaTime);

		screen.newFrame();
	}

	testBlock.cleanup();

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

	chunk.cleanup();

	//lamps.cleanup();

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

	//if (Keyboard::key(GLFW_KEY_W)) {
	//	transform = glm::translate(transform, glm::vec3(0.0f, 0.007f, 0.0f));
	//}

	// change camera

	if (Keyboard::keyWentDown(GLFW_KEY_C)) {
		if (currentCam == &cameras[0]) {
			currentCam = &cameras[1];
		}
		else {
			currentCam = &cameras[0];
		}
	}

	// move camera
	if (Keyboard::key(GLFW_KEY_W)) {
		currentCam->updateCameraPos(CameraDirection::FORWARD, dt);
	}
	if (Keyboard::key(GLFW_KEY_S)) {
		currentCam->updateCameraPos(CameraDirection::BACKWARD, dt);
	}
	if (Keyboard::key(GLFW_KEY_D)) {
		currentCam->updateCameraPos(CameraDirection::RIGHT, dt);
	}
	if (Keyboard::key(GLFW_KEY_A)) {
		currentCam->updateCameraPos(CameraDirection::LEFT, dt);
	}
	if (Keyboard::key(GLFW_KEY_SPACE)) {
		currentCam->updateCameraPos(CameraDirection::UP, dt);
	}
	if (Keyboard::key(GLFW_KEY_LEFT_SHIFT)) {
		currentCam->updateCameraPos(CameraDirection::DOWN, dt);
	}

	double dx = Mouse::getDX();
	double dy = Mouse::getDY();

	if (dx != 0 || dy != 0) {
		currentCam->updateCameraDirection(dx, dy);
	}

	double scrollDy = Mouse::getScrollDY();
	if (scrollDy != 0) {
		currentCam->updateCameraZoom(scrollDy);
	}


	if (Keyboard::keyWentDown(GLFW_KEY_L)) {
		launchItem(dt);
		//sphere.rb.applyImpulse(currentCam->cameraFront, 5.0f, deltaTime);
	}
}