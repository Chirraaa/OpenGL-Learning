#ifndef LAMP_HPP
#define LAMP_HPP

#include "cube.hpp"
#include "../Light.h"

class Lamp : public Cube {
public:

	glm::vec3 lightColor;

	PointLight pointLight;

	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;

	Lamp() = default;

	Lamp(glm::vec3 lightColor,
		glm::vec4 ambient,
		glm::vec4 diffuse,
		glm::vec4 specular,
		float k0,
		float k1,
		float k2,
		glm::vec3 pos,
		glm::vec3 size)
		: lightColor(lightColor), pointLight({pos, k0, k1, k2,ambient,diffuse,specular}), Cube(pos, size) {
	}

	void render(Shader shader) {
		// set light color
		shader.set3Float("lightColor", lightColor);

		Cube::render(shader);
	}

};

#endif