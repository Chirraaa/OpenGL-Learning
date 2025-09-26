#ifndef VOXELCHUNK_HPP
#define VOXELCHUNK_HPP

#include "voxel.hpp"
#include "../../generation/perlin.h"
#include <vector>
#include <array>
#include <unordered_map>
#include <map> // <-- ADD THIS LINE

// Forward declare the struct to avoid circular dependency
struct ChunkMeshData;

enum class VoxelType {
	DIRT = 0,
	COBBLESTONE = 1,
	SAND = 2,
	GRASS = 3,
	AIR = 4
};

struct VoxelTextures {
	Texture diffuse;
	Texture top;
	Texture side;
	Texture bottom;
	bool hasMultipleTextures = false;
};

const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 64;

class VoxelChunk {
public:
	// CORRECTED: Changed to unordered_map to match ChunkMeshData
	std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, VoxelType>>> voxels;

private:
	glm::vec3 chunkPosition;
	std::map<VoxelType, std::vector<Mesh>> chunkMeshes;
	std::map<VoxelType, VoxelTextures> voxelTextures;
	bool texturesLoaded = false;
	bool voxelDataLoaded = false;

public:
	VoxelChunk(glm::vec3 pos, unsigned int seed) : chunkPosition(pos) {}

	~VoxelChunk() {
		cleanup();
	}

	void uploadMesh(const ChunkMeshData& data);
	void render(Shader& shader);
	void cleanup();

	static void addFaceToMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, glm::vec3 localPos, Face face);

	// This old function is kept for compatibility but is now a dummy
	Voxel& getBlock(int x, int y, int z);

	void rebuildMesh();
	void setBlock(int localX, int localY, int localZ, VoxelType type);

	// CORRECTED: Renamed function to avoid overload conflict
	VoxelType getBlockType(int localX, int localY, int localZ);

private:
	void loadTextures();
};

#endif