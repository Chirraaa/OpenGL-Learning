#ifndef LAMP_HPP
#define LAMP_HPP

#include "cube.hpp"
#include "../Light.h"

class Lamp : public Cube {
public:

	glm::vec3 lightColor;

	PointLight pointLight;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	Lamp(glm::vec3 lightColor,
		glm::vec3 diffuse,
		glm::vec3 specular,
		glm::vec3 pos,
		glm::vec3 size)
		: lightColor(lightColor), pointLight({pos,ambient,diffuse,specular}), Cube(Material::white_plastic, pos, size) {
	}

	void render(Shader shader) {
		// set light color
		shader.set3Float("lightColor", lightColor);

		Cube::render(shader);
	}

};

#endif