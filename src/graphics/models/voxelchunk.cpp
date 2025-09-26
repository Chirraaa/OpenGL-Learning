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

void VoxelChunk::uploadMesh(const ChunkMeshData& data) {
	loadTextures();
	cleanup();

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

	// Create meshes for grass
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

void VoxelChunk::addFaceToMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, glm::vec3 localPos, Face face) {
	unsigned int startIndex = vertices.size();
	glm::vec3 facePositions[4];
	glm::vec3 normal;

	// IMPORTANT: Add 0.5 offset to center the block at its coordinate
	glm::vec3 blockCenter = localPos + glm::vec3(0.5f, 0.5f, 0.5f);

	switch (face) {
	case Face::FRONT:
		facePositions[0] = blockCenter + glm::vec3(-0.5f, -0.5f, 0.5f);
		facePositions[1] = blockCenter + glm::vec3(0.5f, -0.5f, 0.5f);
		facePositions[2] = blockCenter + glm::vec3(0.5f, 0.5f, 0.5f);
		facePositions[3] = blockCenter + glm::vec3(-0.5f, 0.5f, 0.5f);
		normal = glm::vec3(0.0f, 0.0f, 1.0f);
		break;
	case Face::BACK:
		facePositions[0] = blockCenter + glm::vec3(0.5f, -0.5f, -0.5f);
		facePositions[1] = blockCenter + glm::vec3(-0.5f, -0.5f, -0.5f);
		facePositions[2] = blockCenter + glm::vec3(-0.5f, 0.5f, -0.5f);
		facePositions[3] = blockCenter + glm::vec3(0.5f, 0.5f, -0.5f);
		normal = glm::vec3(0.0f, 0.0f, -1.0f);
		break;
	case Face::LEFT:
		facePositions[0] = blockCenter + glm::vec3(-0.5f, -0.5f, -0.5f);
		facePositions[1] = blockCenter + glm::vec3(-0.5f, -0.5f, 0.5f);
		facePositions[2] = blockCenter + glm::vec3(-0.5f, 0.5f, 0.5f);
		facePositions[3] = blockCenter + glm::vec3(-0.5f, 0.5f, -0.5f);
		normal = glm::vec3(-1.0f, 0.0f, 0.0f);
		break;
	case Face::RIGHT:
		facePositions[0] = blockCenter + glm::vec3(0.5f, -0.5f, 0.5f);
		facePositions[1] = blockCenter + glm::vec3(0.5f, -0.5f, -0.5f);
		facePositions[2] = blockCenter + glm::vec3(0.5f, 0.5f, -0.5f);
		facePositions[3] = blockCenter + glm::vec3(0.5f, 0.5f, 0.5f);
		normal = glm::vec3(1.0f, 0.0f, 0.0f);
		break;
	case Face::BOTTOM:
		facePositions[0] = blockCenter + glm::vec3(-0.5f, -0.5f, -0.5f);
		facePositions[1] = blockCenter + glm::vec3(0.5f, -0.5f, -0.5f);
		facePositions[2] = blockCenter + glm::vec3(0.5f, -0.5f, 0.5f);
		facePositions[3] = blockCenter + glm::vec3(-0.5f, -0.5f, 0.5f);
		normal = glm::vec3(0.0f, -1.0f, 0.0f);
		break;
	case Face::TOP:
		facePositions[0] = blockCenter + glm::vec3(-0.5f, 0.5f, 0.5f);
		facePositions[1] = blockCenter + glm::vec3(0.5f, 0.5f, 0.5f);
		facePositions[2] = blockCenter + glm::vec3(0.5f, 0.5f, -0.5f);
		facePositions[3] = blockCenter + glm::vec3(-0.5f, 0.5f, -0.5f);
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

// CORRECTED: Removed the duplicate setBlock function
void VoxelChunk::setBlock(int localX, int localY, int localZ, VoxelType type) {
	if (type == VoxelType::AIR) {
		if (voxels.count(localX) && voxels[localX].count(localY) && voxels[localX][localY].count(localZ)) {
			voxels[localX][localY].erase(localZ);
		}
	}
	else {
		voxels[localX][localY][localZ] = type;
	}
}

// CORRECTED: Renamed function to getBlockType
VoxelType VoxelChunk::getBlockType(int localX, int localY, int localZ) {
	if (voxels.count(localX) && voxels[localX].count(localY) && voxels[localX][localY].count(localZ)) {
		return voxels.at(localX).at(localY).at(localZ);
	}
	return VoxelType::AIR;
}

void VoxelChunk::rebuildMesh() {
	ChunkMeshData meshData;

	auto hasVoxel = [&](int x, int y, int z) {
		return voxels.count(x) && voxels.at(x).count(y) && voxels.at(x).at(y).count(z);
		};

	for (const auto& x_pair : voxels) {
		int x = x_pair.first;
		for (const auto& y_pair : x_pair.second) {
			int y = y_pair.first;
			for (const auto& z_pair : y_pair.second) {
				int z = z_pair.first;
				VoxelType currentType = z_pair.second;
				if (currentType == VoxelType::AIR) continue;

				glm::vec3 localVoxelPos(x, y, z);
				if (currentType == VoxelType::GRASS) {
					if (!hasVoxel(x, y + 1, z)) VoxelChunk::addFaceToMeshData(meshData.grassTopVertices, meshData.grassTopIndices, localVoxelPos, Face::TOP);
					if (!hasVoxel(x, y - 1, z)) VoxelChunk::addFaceToMeshData(meshData.grassBottomVertices, meshData.grassBottomIndices, localVoxelPos, Face::BOTTOM);
					if (!hasVoxel(x, y, z + 1)) VoxelChunk::addFaceToMeshData(meshData.grassSideVertices, meshData.grassSideIndices, localVoxelPos, Face::FRONT);
					if (!hasVoxel(x, y, z - 1)) VoxelChunk::addFaceToMeshData(meshData.grassSideVertices, meshData.grassSideIndices, localVoxelPos, Face::BACK);
					if (!hasVoxel(x - 1, y, z)) VoxelChunk::addFaceToMeshData(meshData.grassSideVertices, meshData.grassSideIndices, localVoxelPos, Face::LEFT);
					if (!hasVoxel(x + 1, y, z)) VoxelChunk::addFaceToMeshData(meshData.grassSideVertices, meshData.grassSideIndices, localVoxelPos, Face::RIGHT);
				}
				else {
					int typeIndex = static_cast<int>(currentType);
					if (!hasVoxel(x, y, z + 1)) VoxelChunk::addFaceToMeshData(meshData.simpleVertices[typeIndex], meshData.simpleIndices[typeIndex], localVoxelPos, Face::FRONT);
					if (!hasVoxel(x, y, z - 1)) VoxelChunk::addFaceToMeshData(meshData.simpleVertices[typeIndex], meshData.simpleIndices[typeIndex], localVoxelPos, Face::BACK);
					if (!hasVoxel(x - 1, y, z)) VoxelChunk::addFaceToMeshData(meshData.simpleVertices[typeIndex], meshData.simpleIndices[typeIndex], localVoxelPos, Face::LEFT);
					if (!hasVoxel(x + 1, y, z)) VoxelChunk::addFaceToMeshData(meshData.simpleVertices[typeIndex], meshData.simpleIndices[typeIndex], localVoxelPos, Face::RIGHT);
					if (!hasVoxel(x, y + 1, z)) VoxelChunk::addFaceToMeshData(meshData.simpleVertices[typeIndex], meshData.simpleIndices[typeIndex], localVoxelPos, Face::TOP);
					if (!hasVoxel(x, y - 1, z)) VoxelChunk::addFaceToMeshData(meshData.simpleVertices[typeIndex], meshData.simpleIndices[typeIndex], localVoxelPos, Face::BOTTOM);
				}
			}
		}
	}
	uploadMesh(meshData);
}

// CORRECTED: This function is now a dummy to prevent compile errors.
Voxel& VoxelChunk::getBlock(int x, int y, int z) {
	static Voxel airVoxel; // This function is mostly unused now but kept for compatibility
	return airVoxel;
}