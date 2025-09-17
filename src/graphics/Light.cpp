#include "Light.h"

void PointLight::render(Shader shader) {
	std::string name = "pointLight";
	shader.set3Float(name + ".position", position);

	shader.set3Float(name + ".ambient", ambient);
	shader.set3Float(name + ".diffuse", diffuse);
	shader.set3Float(name + ".specular", specular);
}

void DirLight::render(Shader shader) {
	std::string name = "dirLight";

	shader.set3Float(name + ".direction", direction);
	shader.set3Float(name + ".ambient", ambient);
	shader.set3Float(name + ".diffuse", diffuse);
	shader.set3Float(name + ".specular", specular);
}
