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

#include "graphics/Shader.h"
#include "graphics/Texture.h"
#include "graphics/Model.h"
#include "graphics/Light.h"


#include "graphics/models/cube.hpp"
#include "graphics/models/lamp.hpp"
#include "graphics/models/gun.hpp"


#include "io/Keyboard.h"
#include "io/Mouse.h"
#include "io/Camera.h"
#include "io/Screen.h"



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


float x, y, z;


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

	Gun g;
	g.loadModel("assets/models/lowpoly Rifle/scene.gltf");


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


	DirLight dirLight = { glm::vec3(-0.2f, -1.0f, -0.3f), 
		glm::vec4(0.1f,0.1f,0.1f,0.1),
		glm::vec4(0.4f,0.4f,0.4f,1.0f),
		glm::vec4(0.75f,0.75f,0.75f,1.0f) };


	glm::vec3 pointLightPositions[] = {
			glm::vec3(0.7f,  0.2f,  2.0f),
			glm::vec3(2.3f, -3.3f, -4.0f),
			glm::vec3(-4.0f,  2.0f, -12.0f),
			glm::vec3(0.0f,  0.0f, -3.0f)
	};

	Lamp lamps[4];
	for (unsigned int i = 0; i < 4; i++) {
		lamps[i] = Lamp(glm::vec3(1.0f),
			glm::vec4(0.05f, 0.05f, 0.05f,1.0f),
			glm::vec4(0.8f, 0.8f, 0.8f,1.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
			1.0f, 0.07f, 0.032f,
			pointLightPositions[i], glm::vec3(0.25f));
		lamps[i].init();
	}

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

		for (int i = 0; i < 4; i++) {
			lamps[i].pointLight.render(shader, i);
		}
		shader.setInt("noPointLights", 4);
			
		spotLight.position = currentCam->cameraPos;
		spotLight.direction = currentCam->cameraFront;
		spotLight.render(shader, 0);
		shader.setInt("noSpotLights", 1);

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
		
		//for(int i = 0; i < 10; i++) 
		//	cubes[i].render(shader);

		g.render(shader, currentCam);

		lampShader.activate();
		lampShader.setMat4("view", view);
		lampShader.setMat4("projection", projection);
		for (int i = 0; i < 4; i++) 
			lamps[i].render(lampShader);

		screen.newFrame();
	}

	//for (int i = 0; i < 10; i++)
	//	cubes[i].cleanup();

	g.cleanup();
	for(int i = 0; i < 4; i++)
		lamps[i].cleanup();

	glfwTerminate();
	return 0;
}



// FUNCTIONS



void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	SCREEN_WIDTH = width;
	SCREEN_HEIGHT = height;
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

}