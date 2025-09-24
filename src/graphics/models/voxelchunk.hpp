#ifndef VOXELCHUNK_HPP
#define VOXELCHUNK_HPP

#include "voxel.hpp"
#include "../../generation/perlin.h"
#include <map>

// Enhanced VoxelType enum with grass
enum class VoxelType {
	DIRT = 0,
	COBBLESTONE = 1,
	SAND = 2,
	GRASS = 3
};

// Structure to hold multiple textures for a single voxel type
struct VoxelTextures {
	Texture diffuse;      // Default texture (used for all faces unless overridden)
	Texture top;          // Top face texture (optional)
	Texture side;         // Side face texture (optional)
	Texture bottom;       // Bottom face texture (optional)
	bool hasMultipleTextures = false;
};

class VoxelChunk : public Voxel {
private:
	static constexpr int CHUNK_SIZE = 16;
	static constexpr int CHUNK_HEIGHT = 64;
	std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, VoxelType>>> voxels;
	glm::vec3 chunkPosition;
	std::map<VoxelType, std::vector<Mesh>> chunkMeshes; // Multiple meshes per type for different textures
	std::map<VoxelType, VoxelTextures> voxelTextures;
	bool needsRebuild = true;
	bool texturesLoaded = false;
	PerlinNoise perlinNoise; // Keep for backward compatibility, but World will handle generation

public:
	// UPDATED: Constructor no longer automatically generates terrain
	VoxelChunk(glm::vec3 pos, unsigned int seed = 2362436)
		: chunkPosition(pos), perlinNoise(seed) {
		// Don't generate terrain automatically - let World handle it
	}

	// NEW: Methods for homogeneous world generation
	void clearVoxels() {
		voxels.clear();
		needsRebuild = true;
	}

	void markForRebuild() {
		needsRebuild = true;
	}

	glm::vec3 getChunkPosition() const {
		return chunkPosition;
	}

	// NEW: Get chunk size constants (useful for World class)
	static int getChunkSize() { return CHUNK_SIZE; }
	static int getChunkHeight() { return CHUNK_HEIGHT; }

	void loadTextures() {
		if (texturesLoaded) return;

		// Load simple textures (single texture for all faces)
		voxelTextures[VoxelType::DIRT].diffuse = Texture("assets/textures", "dirt.png", aiTextureType_DIFFUSE);
		voxelTextures[VoxelType::COBBLESTONE].diffuse = Texture("assets/textures", "cobblestone.png", aiTextureType_DIFFUSE);
		voxelTextures[VoxelType::SAND].diffuse = Texture("assets/textures", "sand.png", aiTextureType_DIFFUSE);

		// Load grass textures (multiple textures)
		voxelTextures[VoxelType::GRASS].top = Texture("assets/textures", "grass_block_top.png", aiTextureType_DIFFUSE);
		voxelTextures[VoxelType::GRASS].side = Texture("assets/textures", "grass_block_side.png", aiTextureType_DIFFUSE);
		voxelTextures[VoxelType::GRASS].bottom = Texture("assets/textures", "dirt.png", aiTextureType_DIFFUSE); // Grass bottom is dirt
		voxelTextures[VoxelType::GRASS].hasMultipleTextures = true;

		// Load all textures
		for (auto& pair : voxelTextures) {
			VoxelTextures& textures = pair.second;
			if (textures.hasMultipleTextures) {
				textures.top.load();
				textures.side.load();
				textures.bottom.load();
			}
			else {
				textures.diffuse.load();
			}
		}

		texturesLoaded = true;
	}

	// LEGACY: Keep these terrain generation methods for backward compatibility
	// These won't be used by the World class for homogeneous generation
	void generateTerrain() {
		voxels.clear();

		int seaLevel = 34;

		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				double worldX = chunkPosition.x + x;
				double worldZ = chunkPosition.z + z;

				double noiseValue = perlinNoise.fractalNoise(
					worldX * 0.01,
					worldZ * 0.01,
					4, 0.5, 1.0
				);

				int baseHeight = 32;
				int variation = 16;
				int height = baseHeight + (int)(noiseValue * variation);
				height = std::max(0, std::min(height, CHUNK_HEIGHT - 1));

				if (height < seaLevel) {
					for (int y = 0; y <= height; y++) {
						addVoxel(x, y, z, VoxelType::SAND);
					}
				}
				else {
					for (int y = 0; y <= height; y++) {
						if (y == height) {
							// Top layer is grass
							addVoxel(x, y, z, VoxelType::GRASS);
						}
						else if (y >= height - 3) {
							// Few layers below surface are dirt
							addVoxel(x, y, z, VoxelType::DIRT);
						}
						else {
							// Deep layers are cobblestone
							addVoxel(x, y, z, VoxelType::COBBLESTONE);
						}
					}
				}
			}
		}

		needsRebuild = true;
	}

	void generateSimpleHills() {
		voxels.clear();
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				double worldX = chunkPosition.x + x;
				double worldZ = chunkPosition.z + z;
				double noiseValue = perlinNoise.noise(worldX * 0.05, worldZ * 0.05);
				int height = 20 + (int)(noiseValue * 10);
				height = std::max(0, std::min(height, CHUNK_HEIGHT - 1));
				for (int y = 0; y <= height; y++) {
					if (y == height) {
						addVoxel(x, y, z, VoxelType::GRASS);
					}
					else if (y >= height - 2) {
						addVoxel(x, y, z, VoxelType::DIRT);
					}
					else {
						addVoxel(x, y, z, VoxelType::COBBLESTONE);
					}
				}
			}
		}
		needsRebuild = true;
	}

	void generateMountains() {
		voxels.clear();
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				double worldX = chunkPosition.x + x;
				double worldZ = chunkPosition.z + z;
				double largeFeatures = perlinNoise.noise(worldX * 0.003, worldZ * 0.003) * 30;
				double mediumFeatures = perlinNoise.noise(worldX * 0.01, worldZ * 0.01) * 15;
				double smallFeatures = perlinNoise.noise(worldX * 0.05, worldZ * 0.05) * 5;
				int height = 25 + (int)(largeFeatures + mediumFeatures + smallFeatures);
				height = std::max(0, std::min(height, CHUNK_HEIGHT - 1));
				for (int y = 0; y <= height; y++) {
					if (y == height) {
						addVoxel(x, y, z, VoxelType::GRASS);
					}
					else if (y >= height - 3) {
						addVoxel(x, y, z, VoxelType::DIRT);
					}
					else {
						addVoxel(x, y, z, VoxelType::COBBLESTONE);
					}
				}
			}
		}
		needsRebuild = true;
	}

	void generateCaves() {
		voxels.clear();
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				double worldX = chunkPosition.x + x;
				double worldZ = chunkPosition.z + z;
				double surfaceNoise = perlinNoise.noise(worldX * 0.01, worldZ * 0.01);
				int surfaceHeight = 30 + (int)(surfaceNoise * 15);
				for (int y = 0; y < CHUNK_HEIGHT; y++) {
					double worldY = chunkPosition.y + y;
					if (y <= surfaceHeight) {
						double caveNoise = perlinNoise.noise(worldX * 0.02, (worldZ + y * 10) * 0.02);
						if (caveNoise > -0.3) {
							if (y == surfaceHeight) {
								addVoxel(x, y, z, VoxelType::GRASS);
							}
							else if (y >= surfaceHeight - 8) {
								addVoxel(x, y, z, VoxelType::DIRT);
							}
							else {
								addVoxel(x, y, z, VoxelType::COBBLESTONE);
							}
						}
					}
				}
			}
		}
		needsRebuild = true;
	}

	int hashPos(int x, int y, int z) const {
		return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
	}

	void addVoxel(int x, int y, int z, VoxelType type = VoxelType::DIRT) {
		if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
			voxels[x][y][z] = type;
			needsRebuild = true;
		}
	}

	void removeVoxel(int x, int y, int z) {
		if (voxels.count(x) && voxels[x].count(y) && voxels[x][y].count(z)) {
			voxels[x][y].erase(z);
			if (voxels[x][y].empty()) {
				voxels[x].erase(y);
				if (voxels[x].empty()) {
					voxels.erase(x);
				}
			}
			needsRebuild = true;
		}
	}

	bool hasVoxel(int x, int y, int z) const {
		return voxels.count(x) && voxels.at(x).count(y) && voxels.at(x).at(y).count(z);
	}

	VoxelType getVoxelType(int x, int y, int z) const {
		if (hasVoxel(x, y, z)) {
			return voxels.at(x).at(y).at(z);
		}
		return VoxelType::DIRT; // Default fallback
	}

	// NEW: Method to get voxel type by reference (for World class)
	VoxelType* getVoxelTypePtr(int x, int y, int z) {
		if (hasVoxel(x, y, z)) {
			return &voxels[x][y][z];
		}
		return nullptr;
	}

	// NEW: Get total number of voxels in chunk (for debugging)
	size_t getVoxelCount() const {
		size_t count = 0;
		for (const auto& x_pair : voxels) {
			for (const auto& y_pair : x_pair.second) {
				count += y_pair.second.size();
			}
		}
		return count;
	}

	void rebuildMesh() {
		if (!needsRebuild) return;

		// Load textures if not already loaded
		loadTextures();

		// Clean up existing meshes
		for (auto& pair : chunkMeshes) {
			for (auto& mesh : pair.second) {
				mesh.clearnup();
			}
		}
		chunkMeshes.clear();

		// For simple voxel types (single texture)
		std::map<VoxelType, std::vector<Vertex>> simpleVertices;
		std::map<VoxelType, std::vector<unsigned int>> simpleIndices;

		// For grass voxel type (multiple meshes for different face textures)
		std::vector<Vertex> grassTopVertices, grassSideVertices, grassBottomVertices;
		std::vector<unsigned int> grassTopIndices, grassSideIndices, grassBottomIndices;

		for (const auto& x_pair : voxels) {
			int x = x_pair.first;
			for (const auto& y_pair : x_pair.second) {
				int y = y_pair.first;
				for (const auto& z_pair : y_pair.second) {
					int z = z_pair.first;
					VoxelType currentType = z_pair.second;
					glm::vec3 localVoxelPos = glm::vec3(x, y, z);

					if (currentType == VoxelType::GRASS) {
						// Handle grass specially with different textures per face
						if (!hasVoxel(x, y + 1, z)) addGrassFace(grassTopVertices, grassTopIndices, localVoxelPos, Face::TOP);
						if (!hasVoxel(x, y - 1, z)) addGrassFace(grassBottomVertices, grassBottomIndices, localVoxelPos, Face::BOTTOM);

						// Side faces
						if (!hasVoxel(x, y, z + 1)) addGrassFace(grassSideVertices, grassSideIndices, localVoxelPos, Face::FRONT);
						if (!hasVoxel(x, y, z - 1)) addGrassFace(grassSideVertices, grassSideIndices, localVoxelPos, Face::BACK);
						if (!hasVoxel(x - 1, y, z)) addGrassFace(grassSideVertices, grassSideIndices, localVoxelPos, Face::LEFT);
						if (!hasVoxel(x + 1, y, z)) addGrassFace(grassSideVertices, grassSideIndices, localVoxelPos, Face::RIGHT);
					}
					else {
						// Handle simple voxel types
						auto& currentVertices = simpleVertices[currentType];
						auto& currentIndices = simpleIndices[currentType];

						if (!hasVoxel(x, y, z + 1)) addVoxelFace(currentVertices, currentIndices, localVoxelPos, Face::FRONT);
						if (!hasVoxel(x, y, z - 1)) addVoxelFace(currentVertices, currentIndices, localVoxelPos, Face::BACK);
						if (!hasVoxel(x - 1, y, z)) addVoxelFace(currentVertices, currentIndices, localVoxelPos, Face::LEFT);
						if (!hasVoxel(x + 1, y, z)) addVoxelFace(currentVertices, currentIndices, localVoxelPos, Face::RIGHT);
						if (!hasVoxel(x, y + 1, z)) addVoxelFace(currentVertices, currentIndices, localVoxelPos, Face::TOP);
						if (!hasVoxel(x, y - 1, z)) addVoxelFace(currentVertices, currentIndices, localVoxelPos, Face::BOTTOM);
					}
				}
			}
		}

		// Create meshes for simple voxel types
		for (auto& pair : simpleVertices) {
			VoxelType type = pair.first;
			const auto& v_list = pair.second;
			if (!v_list.empty()) {
				Mesh newMesh(v_list, simpleIndices.at(type));
				newMesh.textures.push_back(voxelTextures.at(type).diffuse);
				newMesh.setUseTexture(true);
				chunkMeshes[type].push_back(newMesh);
			}
		}

		// Create meshes for grass (multiple meshes for different textures)
		if (!grassTopVertices.empty()) {
			Mesh topMesh(grassTopVertices, grassTopIndices);
			topMesh.textures.push_back(voxelTextures.at(VoxelType::GRASS).top);
			topMesh.setUseTexture(true);
			chunkMeshes[VoxelType::GRASS].push_back(topMesh);
		}

		if (!grassSideVertices.empty()) {
			Mesh sideMesh(grassSideVertices, grassSideIndices);
			sideMesh.textures.push_back(voxelTextures.at(VoxelType::GRASS).side);
			sideMesh.setUseTexture(true);
			chunkMeshes[VoxelType::GRASS].push_back(sideMesh);
		}

		if (!grassBottomVertices.empty()) {
			Mesh bottomMesh(grassBottomVertices, grassBottomIndices);
			bottomMesh.textures.push_back(voxelTextures.at(VoxelType::GRASS).bottom);
			bottomMesh.setUseTexture(true);
			chunkMeshes[VoxelType::GRASS].push_back(bottomMesh);
		}

		needsRebuild = false;
	}

	void render(Shader shader) {
		rebuildMesh();

		if (chunkMeshes.empty()) {
			return;
		}

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, chunkPosition);
		shader.setMat4("model", model);
		shader.setFloat("material.shininess", 32.0f);

		// Disable grass tinting for all meshes
		shader.setBool("useGrassTint", false);

		// Render each voxel type
		for (auto& pair : chunkMeshes) {
			VoxelType voxelType = pair.first;

			for (size_t i = 0; i < pair.second.size(); i++) {
				auto& mesh = pair.second[i];

				// Check if this is grass top mesh (first mesh for grass type)
				if (voxelType == VoxelType::GRASS && i == 0) {
					// Enable grass tint for grass top
					shader.setInt("applyGrassTint", 1);
					shader.set3Float("grassTintColor", 0.6f, 1.0f, 0.4f); // Green tint
				}
				else {
					// Disable tint for all other meshes
					shader.setInt("applyGrassTint", 0);
				}

				mesh.render(shader);
			}
		}
	}

	void cleanup() {
		for (auto& pair : chunkMeshes) {
			for (auto& mesh : pair.second) {
				mesh.clearnup();
			}
		}
		chunkMeshes.clear();
		voxels.clear();
	}

private:
	void addVoxelFace(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
		glm::vec3 localPos, Face face) {
		unsigned int startIndex = vertices.size();
		std::vector<glm::vec3> facePositions;
		glm::vec3 normal;

		switch (face) {
		case Face::FRONT:
			facePositions = {
				localPos + glm::vec3(-0.5f, -0.5f,  0.5f),
				localPos + glm::vec3(0.5f, -0.5f,  0.5f),
				localPos + glm::vec3(0.5f,  0.5f,  0.5f),
				localPos + glm::vec3(-0.5f,  0.5f,  0.5f)
			};
			normal = glm::vec3(0.0f, 0.0f, 1.0f);
			break;
		case Face::BACK:
			facePositions = {
				localPos + glm::vec3(0.5f, -0.5f, -0.5f),
				localPos + glm::vec3(-0.5f, -0.5f, -0.5f),
				localPos + glm::vec3(-0.5f,  0.5f, -0.5f),
				localPos + glm::vec3(0.5f,  0.5f, -0.5f)
			};
			normal = glm::vec3(0.0f, 0.0f, -1.0f);
			break;
		case Face::LEFT:
			facePositions = {
				localPos + glm::vec3(-0.5f, -0.5f, -0.5f),
				localPos + glm::vec3(-0.5f, -0.5f,  0.5f),
				localPos + glm::vec3(-0.5f,  0.5f,  0.5f),
				localPos + glm::vec3(-0.5f,  0.5f, -0.5f)
			};
			normal = glm::vec3(-1.0f, 0.0f, 0.0f);
			break;
		case Face::RIGHT:
			facePositions = {
				localPos + glm::vec3(0.5f, -0.5f,  0.5f),
				localPos + glm::vec3(0.5f, -0.5f, -0.5f),
				localPos + glm::vec3(0.5f,  0.5f, -0.5f),
				localPos + glm::vec3(0.5f,  0.5f,  0.5f)
			};
			normal = glm::vec3(1.0f, 0.0f, 0.0f);
			break;
		case Face::BOTTOM:
			facePositions = {
				localPos + glm::vec3(-0.5f, -0.5f, -0.5f),
				localPos + glm::vec3(0.5f, -0.5f, -0.5f),
				localPos + glm::vec3(0.5f, -0.5f,  0.5f),
				localPos + glm::vec3(-0.5f, -0.5f,  0.5f)
			};
			normal = glm::vec3(0.0f, -1.0f, 0.0f);
			break;
		case Face::TOP:
			facePositions = {
				localPos + glm::vec3(-0.5f,  0.5f,  0.5f),
				localPos + glm::vec3(0.5f,  0.5f,  0.5f),
				localPos + glm::vec3(0.5f,  0.5f, -0.5f),
				localPos + glm::vec3(-0.5f,  0.5f, -0.5f)
			};
			normal = glm::vec3(0.0f, 1.0f, 0.0f);
			break;
		}

		std::vector<glm::vec2> texCoords = {
			{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
		};

		for (int i = 0; i < 4; i++) {
			Vertex vertex;
			vertex.pos = facePositions[i];
			vertex.normal = normal;
			vertex.texCoord = texCoords[i];
			vertices.push_back(vertex);
		}

		indices.insert(indices.end(), {
			startIndex, startIndex + 1, startIndex + 2,
			startIndex + 2, startIndex + 3, startIndex
			});
	}

	// Same as addVoxelFace but used specifically for grass faces
	void addGrassFace(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
		glm::vec3 localPos, Face face) {
		addVoxelFace(vertices, indices, localPos, face);
	}
};

#endif