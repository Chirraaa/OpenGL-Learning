#ifndef WORLD_H
#define WORLD_H

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include <glm/glm.hpp>
#include "../../generation/perlin.h"
#include "../models/ThreadSafeQueue.hpp"
#include "../models/voxelchunk.hpp"

// Forward declarations
class Shader;
enum class VoxelType;

struct Block {
	VoxelType type;

	Block() : type(VoxelType::AIR) {}
	Block(VoxelType t) : type(t) {}

	bool isAir() const { return type == VoxelType::AIR; }
	bool isSolid() const { return !isAir(); }
};

struct ChunkMeshData {
	long long chunkKey;
	glm::vec3 chunkPosition;
	std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, VoxelType>>> voxels;

	std::vector<Vertex> simpleVertices[4];
	std::vector<unsigned int> simpleIndices[4];
	std::vector<Vertex> grassTopVertices;
	std::vector<unsigned int> grassTopIndices;
	std::vector<Vertex> grassSideVertices;
	std::vector<unsigned int> grassSideIndices;
	std::vector<Vertex> grassBottomVertices;
	std::vector<unsigned int> grassBottomIndices;
};

class World {
public:
	World(int renderDist = 5, unsigned int seed = 12345);
	~World();

	const int CHUNK_SIZE = 16;
	const int CHUNK_HEIGHT = 32;

	void update(glm::vec3 playerPos);
	void render(Shader& shader);
	VoxelChunk* getChunk(int chunkX, int chunkZ);
	void cleanup();

	void setRenderDistance(int distance) { renderDistance = distance; }
	int getRenderDistance() const { return renderDistance; }

	float getTerrainHeight(float worldX, float worldZ);
	VoxelType getBlockType(float worldX, float worldY, float worldZ, float terrainHeight);

	void setBlock(int worldX, int worldY, int worldZ, VoxelType type);
	void placeBlock(int worldX, int worldY, int worldZ, VoxelType type);
	VoxelType getBlockTypeAt(int worldX, int worldY, int worldZ);

	void getChunkCoords(glm::vec3 worldPos, int& chunkX, int& chunkZ);

	size_t getLoadedChunkCount() const {
		return chunks.size();
	}
private:
	std::unordered_map<long long, std::unique_ptr<VoxelChunk>> chunks;
	int renderDistance;
	unsigned int worldSeed;
	PerlinNoise worldNoise;
	glm::vec3 lastPlayerPos;

	std::vector<std::thread> m_chunkWorkers;
	std::atomic<bool> m_isRunning;

	ThreadSafeQueue<glm::ivec2> m_chunksToLoadQueue;
	ThreadSafeQueue<ChunkMeshData> m_meshesToUploadQueue;

	std::mutex m_worldMutex;
	std::unordered_set<long long> m_generatingChunks;

	void chunkWorkerLoop();

	long long getChunkKey(int chunkX, int chunkZ);

	void generateChunksAroundPosition(glm::vec3 pos);
	void unloadDistantChunks(glm::vec3 playerPos);
	bool shouldLoadChunk(int chunkX, int chunkZ, int playerChunkX, int playerChunkZ, int maxDistance = -1);
};

#endif