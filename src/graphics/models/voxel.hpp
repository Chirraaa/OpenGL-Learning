#ifndef VOXEL_HPP
#define VOXEL_HPP

#include "../Model.h"
#include "../Texture.h"
#include "cube.hpp"
#include "modelarray.hpp"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <array>

// Face directions for culling
enum class Face {
	FRONT = 0,
	BACK = 1,
	LEFT = 2,
	RIGHT = 3,
	TOP = 4,
	BOTTOM = 5
};

class Voxel : public Cube {
public:
	// Bitmask for which faces to render (1 = render, 0 = cull)
	uint8_t visibleFaces = 0b111111; // All faces visible by default

	using Cube::Cube;

	void init() {
		// Don't call Cube::init() - we'll create our own mesh
		createMeshWithCulling();

		Texture dirtTexture("assets/textures", "dirt.png", aiTextureType_DIFFUSE);
		dirtTexture.load();

		if (!meshes.empty()) {
			meshes[0].textures.push_back(dirtTexture);
			meshes[0].setUseTexture(true);
		}
	}

	// Set which faces should be visible
	void setVisibleFaces(uint8_t faces) {
		visibleFaces = faces;
		// Recreate mesh if it already exists
		if (!meshes.empty()) {
			meshes.clear();
			createMeshWithCulling();
		}
		Texture dirtTexture("assets/textures", "dirt.png", aiTextureType_DIFFUSE);
		dirtTexture.load();
		meshes[0].textures.push_back(dirtTexture);
		meshes[0].setUseTexture(true);
	}

	// Check if a specific face is visible
	bool isFaceVisible(Face face) const {
		return (visibleFaces >> static_cast<int>(face)) & 1;
	}

	// Set a specific face visibility
	void setFaceVisible(Face face, bool visible) {
		if (visible) {
			visibleFaces |= (1 << static_cast<int>(face));
		}
		else {
			visibleFaces &= ~(1 << static_cast<int>(face));
		}
	}

private:
	void createMeshWithCulling() {
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		// Face vertex data - each face has 4 vertices
		// Format: position(3), normal(3), texcoord(2)

		// Front face
		if (isFaceVisible(Face::FRONT)) {
			addFaceVertices(vertices, indices, {
				{-0.5f, -0.5f,  0.5f,     0.0f,  0.0f,  1.0f,    0.0f, 0.0f},
				{ 0.5f, -0.5f,  0.5f,     0.0f,  0.0f,  1.0f,    1.0f, 0.0f},
				{ 0.5f,  0.5f,  0.5f,     0.0f,  0.0f,  1.0f,    1.0f, 1.0f},
				{-0.5f,  0.5f,  0.5f,     0.0f,  0.0f,  1.0f,    0.0f, 1.0f}
				});
		}

		// Back face
		if (isFaceVisible(Face::BACK)) {
			addFaceVertices(vertices, indices, {
				{ 0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,    0.0f, 0.0f},
				{-0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,    1.0f, 0.0f},
				{-0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,    1.0f, 1.0f},
				{ 0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,    0.0f, 1.0f}
				});
		}

		// Left face
		if (isFaceVisible(Face::LEFT)) {
			addFaceVertices(vertices, indices, {
				{-0.5f, -0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,    0.0f, 0.0f},
				{-0.5f, -0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,    1.0f, 0.0f},
				{-0.5f,  0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,    1.0f, 1.0f},
				{-0.5f,  0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,    0.0f, 1.0f}
				});
		}

		// Right face
		if (isFaceVisible(Face::RIGHT)) {
			addFaceVertices(vertices, indices, {
				{ 0.5f, -0.5f,  0.5f,     1.0f,  0.0f,  0.0f,    0.0f, 0.0f},
				{ 0.5f, -0.5f, -0.5f,     1.0f,  0.0f,  0.0f,    1.0f, 0.0f},
				{ 0.5f,  0.5f, -0.5f,     1.0f,  0.0f,  0.0f,    1.0f, 1.0f},
				{ 0.5f,  0.5f,  0.5f,     1.0f,  0.0f,  0.0f,    0.0f, 1.0f}
				});
		}

		// Bottom face
		if (isFaceVisible(Face::BOTTOM)) {
			addFaceVertices(vertices, indices, {
				{-0.5f, -0.5f, -0.5f,     0.0f, -1.0f,  0.0f,    0.0f, 1.0f},
				{ 0.5f, -0.5f, -0.5f,     0.0f, -1.0f,  0.0f,    1.0f, 1.0f},
				{ 0.5f, -0.5f,  0.5f,     0.0f, -1.0f,  0.0f,    1.0f, 0.0f},
				{-0.5f, -0.5f,  0.5f,     0.0f, -1.0f,  0.0f,    0.0f, 0.0f}
				});
		}

		// Top face
		if (isFaceVisible(Face::TOP)) {
			addFaceVertices(vertices, indices, {
				{-0.5f,  0.5f,  0.5f,     0.0f,  1.0f,  0.0f,    0.0f, 0.0f},
				{ 0.5f,  0.5f,  0.5f,     0.0f,  1.0f,  0.0f,    1.0f, 0.0f},
				{ 0.5f,  0.5f, -0.5f,     0.0f,  1.0f,  0.0f,    1.0f, 1.0f},
				{-0.5f,  0.5f, -0.5f,     0.0f,  1.0f,  0.0f,    0.0f, 1.0f}
				});
		}

		if (!vertices.empty()) {
			meshes.push_back(Mesh(vertices, indices));
		}
	}

	void addFaceVertices(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
		const std::vector<std::array<float, 8>>& faceData) {
		unsigned int startIndex = vertices.size();

		// Add the 4 vertices for this face
		for (const auto& vertexData : faceData) {
			Vertex vertex;
			vertex.pos = glm::vec3(vertexData[0], vertexData[1], vertexData[2]);
			vertex.normal = glm::vec3(vertexData[3], vertexData[4], vertexData[5]);
			vertex.texCoord = glm::vec2(vertexData[6], vertexData[7]);
			vertices.push_back(vertex);
		}

		// Add indices for two triangles (quad)
		indices.insert(indices.end(), {
			startIndex, startIndex + 1, startIndex + 2,
			startIndex + 2, startIndex + 3, startIndex
			});
	}
};

class VoxelArray : public ModelArray<Voxel> {
public:
	void init() {
		model = Voxel(glm::vec3(0.0f), glm::vec3(1.0f));
		model.init();
	}

	// Method to update face culling for all voxels based on their positions
	void updateFaceCulling() {
		// Create a set of occupied positions for quick lookup
		std::unordered_set<std::string> occupiedPositions;

		for (const auto& rb : instances) {
			glm::ivec3 pos = glm::ivec3(glm::round(rb.pos));
			occupiedPositions.insert(std::to_string(pos.x) + "," +
				std::to_string(pos.y) + "," +
				std::to_string(pos.z));
		}

		// Update each voxel's visible faces
		for (size_t i = 0; i < instances.size(); i++) {
			glm::ivec3 pos = glm::ivec3(glm::round(instances[i].pos));
			uint8_t visibleFaces = 0b111111; // Start with all faces visible

			// Check each direction for adjacent voxels
			std::string posKey;

			// Front face (positive Z)
			posKey = std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z + 1);
			if (occupiedPositions.count(posKey)) visibleFaces &= ~(1 << static_cast<int>(Face::FRONT));

			// Back face (negative Z)
			posKey = std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z - 1);
			if (occupiedPositions.count(posKey)) visibleFaces &= ~(1 << static_cast<int>(Face::BACK));

			// Left face (negative X)
			posKey = std::to_string(pos.x - 1) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z);
			if (occupiedPositions.count(posKey)) visibleFaces &= ~(1 << static_cast<int>(Face::LEFT));

			// Right face (positive X)
			posKey = std::to_string(pos.x + 1) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z);
			if (occupiedPositions.count(posKey)) visibleFaces &= ~(1 << static_cast<int>(Face::RIGHT));

			// Bottom face (negative Y)
			posKey = std::to_string(pos.x) + "," + std::to_string(pos.y - 1) + "," + std::to_string(pos.z);
			if (occupiedPositions.count(posKey)) visibleFaces &= ~(1 << static_cast<int>(Face::BOTTOM));

			// Top face (positive Y)
			posKey = std::to_string(pos.x) + "," + std::to_string(pos.y + 1) + "," + std::to_string(pos.z);
			if (occupiedPositions.count(posKey)) visibleFaces &= ~(1 << static_cast<int>(Face::TOP));

			// Update the voxel's visible faces
			model.setVisibleFaces(visibleFaces);
		}
	}
};

#endif