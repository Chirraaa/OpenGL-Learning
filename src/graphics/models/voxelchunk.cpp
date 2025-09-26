#include "voxelchunk.hpp"
#include "../Shader.h"
#include "../env/World.h"// Needed for ChunkMeshData definition

void VoxelChunk::loadTextures() {
	if (texturesLoaded) return;

	// Load simple textures
	voxelTextures[VoxelType::DIRT].diffuse = Texture("assets/textures", "dirt.png", aiTextureType_DIFFUSE);
	voxelTextures[VoxelType::DIRT].diffuse.load();
	voxelTextures[VoxelType::COBBLESTONE].diffuse = Texture("assets/textures", "cobblestone.png", aiTextureType_DIFFUSE);
	voxelTextures[VoxelType::COBBLESTONE].diffuse.load();
	voxelTextures[VoxelType::SAND].diffuse = Texture("assets/textures", "sand.png", aiTextureType_DIFFUSE);
	voxelTextures[VoxelType::SAND].diffuse.load();

	// Load grass textures
	voxelTextures[VoxelType::GRASS].top = Texture("assets/textures", "grass_block_top.png", aiTextureType_DIFFUSE);
	voxelTextures[VoxelType::GRASS].top.load();
	voxelTextures[VoxelType::GRASS].side = Texture("assets/textures", "grass_block_side.png", aiTextureType_DIFFUSE);
	voxelTextures[VoxelType::GRASS].side.load();
	voxelTextures[VoxelType::GRASS].bottom = Texture("assets/textures", "dirt.png", aiTextureType_DIFFUSE);
	voxelTextures[VoxelType::GRASS].bottom.load();
	voxelTextures[VoxelType::GRASS].hasMultipleTextures = true;

	texturesLoaded = true;
}

// NEW: The implementation of the mesh upload. This happens on the main thread.
void VoxelChunk::uploadMesh(const ChunkMeshData& data) {
	loadTextures(); // Load textures if they aren't already
	cleanup(); // Clear any old mesh data

	// Create meshes for simple voxel types
	for (int i = 0; i < 4; ++i) {
		if (!data.simpleVertices[i].empty()) {
			VoxelType type = static_cast<VoxelType>(i);
			Mesh newMesh(data.simpleVertices[i], data.simpleIndices[i]);
			newMesh.textures.push_back(voxelTextures.at(type).diffuse);
			newMesh.setUseTexture(true);
			chunkMeshes[type].push_back(newMesh);
		}
	}

	// Create meshes for grass (multiple meshes for different textures)
	if (!data.grassTopVertices.empty()) {
		Mesh topMesh(data.grassTopVertices, data.grassTopIndices);
		topMesh.textures.push_back(voxelTextures.at(VoxelType::GRASS).top);
		topMesh.setUseTexture(true);
		chunkMeshes[VoxelType::GRASS].push_back(topMesh);
	}
	if (!data.grassSideVertices.empty()) {
		Mesh sideMesh(data.grassSideVertices, data.grassSideIndices);
		sideMesh.textures.push_back(voxelTextures.at(VoxelType::GRASS).side);
		sideMesh.setUseTexture(true);
		chunkMeshes[VoxelType::GRASS].push_back(sideMesh);
	}
	if (!data.grassBottomVertices.empty()) {
		Mesh bottomMesh(data.grassBottomVertices, data.grassBottomIndices);
		bottomMesh.textures.push_back(voxelTextures.at(VoxelType::GRASS).bottom);
		bottomMesh.setUseTexture(true);
		chunkMeshes[VoxelType::GRASS].push_back(bottomMesh);
	}
}


void VoxelChunk::render(Shader& shader) {
	if (chunkMeshes.empty()) {
		return;
	}

	glm::mat4 model = glm::translate(glm::mat4(1.0f), chunkPosition);
	shader.setMat4("model", model);
	shader.setFloat("material.shininess", 32.0f);
	shader.setInt("applyGrassTint", 0);

	for (auto& pair : chunkMeshes) {
		VoxelType voxelType = pair.first;
		for (size_t i = 0; i < pair.second.size(); i++) {
			auto& mesh = pair.second[i];

			// This logic is a bit fragile, assuming the first grass mesh is the top.
			// A more robust solution might involve storing mesh types.
			if (voxelType == VoxelType::GRASS && mesh.textures[0].path == "grass_block_top.png") {
				shader.setInt("applyGrassTint", 1);
				shader.set3Float("grassTintColor", 0.6f, 1.0f, 0.4f);
			}
			else {
				shader.setInt("applyGrassTint", 0);
			}
			mesh.render(shader);
		}
	}
}

void VoxelChunk::cleanup() {
	for (auto& pair : chunkMeshes) {
		for (auto& mesh : pair.second) {
			mesh.clearnup();
		}
	}
	chunkMeshes.clear();
}

// NEW: Static helper function implementation
void VoxelChunk::addFaceToMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, glm::vec3 localPos, Face face) {
	unsigned int startIndex = vertices.size();
	glm::vec3 facePositions[4];
	glm::vec3 normal;

	// (Vertex positions and normals calculated same as before)
	switch (face) {
	case Face::FRONT:
		facePositions[0] = localPos + glm::vec3(-0.5f, -0.5f, 0.5f);
		facePositions[1] = localPos + glm::vec3(0.5f, -0.5f, 0.5f);
		facePositions[2] = localPos + glm::vec3(0.5f, 0.5f, 0.5f);
		facePositions[3] = localPos + glm::vec3(-0.5f, 0.5f, 0.5f);
		normal = glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	case Face::BACK:
		facePositions[0] = localPos + glm::vec3(0.5f, -0.5f, -0.5f);
		facePositions[1] = localPos + glm::vec3(-0.5f, -0.5f, -0.5f);
		facePositions[2] = localPos + glm::vec3(-0.5f, 0.5f, -0.5f);
		facePositions[3] = localPos + glm::vec3(0.5f, 0.5f, -0.5f);
		normal = glm::vec3(0.0f, 0.0f, -1.0f);
		break;
	case Face::LEFT:
		facePositions[0] = localPos + glm::vec3(-0.5f, -0.5f, -0.5f);
		facePositions[1] = localPos + glm::vec3(-0.5f, -0.5f, 0.5f);
		facePositions[2] = localPos + glm::vec3(-0.5f, 0.5f, 0.5f);
		facePositions[3] = localPos + glm::vec3(-0.5f, 0.5f, -0.5f);
		normal = glm::vec3(-1.0f, 0.0f, 0.0f);
		break;
	case Face::RIGHT:
		facePositions[0] = localPos + glm::vec3(0.5f, -0.5f, 0.5f);
		facePositions[1] = localPos + glm::vec3(0.5f, -0.5f, -0.5f);
		facePositions[2] = localPos + glm::vec3(0.5f, 0.5f, -0.5f);
		facePositions[3] = localPos + glm::vec3(0.5f, 0.5f, 0.5f);
		normal = glm::vec3(1.0f, 0.0f, 0.0f);
		break;
	case Face::BOTTOM:
		facePositions[0] = localPos + glm::vec3(-0.5f, -0.5f, -0.5f);
		facePositions[1] = localPos + glm::vec3(0.5f, -0.5f, -0.5f);
		facePositions[2] = localPos + glm::vec3(0.5f, -0.5f, 0.5f);
		facePositions[3] = localPos + glm::vec3(-0.5f, -0.5f, 0.5f);
		normal = glm::vec3(0.0f, -1.0f, 0.0f);
		break;
	case Face::TOP:
		facePositions[0] = localPos + glm::vec3(-0.5f, 0.5f, 0.5f);
		facePositions[1] = localPos + glm::vec3(0.5f, 0.5f, 0.5f);
		facePositions[2] = localPos + glm::vec3(0.5f, 0.5f, -0.5f);
		facePositions[3] = localPos + glm::vec3(-0.5f, 0.5f, -0.5f);
		normal = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	}

	glm::vec2 texCoords[] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

	for (int i = 0; i < 4; i++) {
		vertices.push_back({ facePositions[i], normal, texCoords[i] });
	}

	indices.insert(indices.end(), {
		startIndex, startIndex + 1, startIndex + 2,
		startIndex + 2, startIndex + 3, startIndex
		});
}

Voxel& VoxelChunk::getBlock(int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
		static Voxel airVoxel;
		return airVoxel;
	}
	return blocks[x][y][z];
}