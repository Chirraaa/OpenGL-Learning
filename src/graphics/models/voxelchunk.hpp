#ifndef VOXELCHUNK_HPP
#define VOXELCHUNK_HPP

#include "voxel.hpp"
#include "../../generation/perlin.h"
#include <map>
#include <vector>
#include <array>

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

class VoxelChunk { // MODIFIED: No longer inherits from Voxel
private:

	glm::vec3 chunkPosition;
	std::map<VoxelType, std::vector<Mesh>> chunkMeshes;
	std::map<VoxelType, VoxelTextures> voxelTextures;
	bool texturesLoaded = false;

	std::array<std::array<std::array<Voxel, CHUNK_SIZE>, CHUNK_HEIGHT>, CHUNK_SIZE> blocks;
	bool voxelDataLoaded = false;
public:
	// MODIFIED: Constructor is much simpler now.
	VoxelChunk(glm::vec3 pos, unsigned int seed) : chunkPosition(pos) {}

	// NEW: Destructor to ensure mesh cleanup
	~VoxelChunk() {
		cleanup();
	}

	// NEW: This method is called by the main thread to create the OpenGL objects.
	void uploadMesh(const ChunkMeshData& data);

	// MODIFIED: Render is simpler, no longer calls rebuildMesh().
	void render(Shader& shader);

	void cleanup();

	// This helper function is now static so worker threads can use it without a VoxelChunk instance.
	static void addFaceToMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, glm::vec3 localPos, Face face);

	Voxel& getBlock(int x, int y, int z);


private:
	void loadTextures();
};

#endif