#include "Mesh.h"

std::vector<Vertex> Vertex::genList(float* vertices, int noVertices) {
	std::vector<Vertex> ret(noVertices);

	int stride = sizeof(Vertex) / sizeof(float);

	for (int i = 0; i < noVertices; i++) {
		ret[i].pos = glm::vec3(
			vertices[i * stride + 0],
			vertices[i * stride + 1],
			vertices[i * stride + 2]
		);

		ret[i].normal = glm::vec3(
			vertices[i * stride + 3],
			vertices[i * stride + 4],
			vertices[i * stride + 5]
		);

		ret[i].texCoord = glm::vec2(
			vertices[i * stride + 6],
			vertices[i * stride + 7]
		);
	}

	return ret;
}

Mesh::Mesh() {}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
	: vertices(vertices), indices(indices), textures(textures) {

	noTex = textures.empty();

	setup();
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, aiColor4D diffuse, aiColor4D specular) :
	vertices(vertices), indices(indices), diffuse(diffuse), specular(specular), noTex(true) {
	setup();
}

void Mesh::render(Shader shader) {
	if (noTex) {
		shader.set4Float("material.diffuse", diffuse);
		shader.set4Float("material.specular", specular);
		shader.setInt("noTex", 1);
	}
	else {

		shader.setInt("noTex", 0);

		// textures
		unsigned int diffuseIdx = 0;
		unsigned int specularIdx = 0;

		for (unsigned int i = 0; i < textures.size(); i++) {
			glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
			// retrieve texture number (the N in diffuse_textureN)
			std::string name;
			switch (textures[i].type) {
			case aiTextureType_DIFFUSE:
				name = "diffuse" + std::to_string(diffuseIdx++);
				break;
			case aiTextureType_SPECULAR:
				name = "specular" + std::to_string(specularIdx++);
				break;
			}
			shader.setInt(name, i);
			textures[i].bind();
		}
	}

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glActiveTexture(GL_TEXTURE0);
}

void Mesh::clearnup() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void Mesh::setup() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// set vertex attribute pointers
	// vertex.position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

	//vertex normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	glEnableVertexAttribArray(2);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

	glBindVertexArray(0);
}

void Mesh::setUseTexture(bool useTexture) {
	// If useTexture is true, we want noTex to be false.
	// If useTexture is false, we want noTex to be true.
	noTex = !useTexture;
}