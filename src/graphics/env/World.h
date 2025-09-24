#ifndef WORLD_H
#define WORLD_H

#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include "../../generation/perlin.h" // Include PerlinNoise

// Forward declarations
class VoxelChunk;
class Shader;
enum class VoxelType;

class World {
public:
	World(int renderDist = 5, unsigned int seed = 12345);
	~World();

	// Update world based on player position (generate/destroy chunks)
	void update(glm::vec3 playerPos);

	// Render all loaded chunks
	void render(Shader& shader);

	// Get a specific chunk (returns nullptr if not loaded)
	VoxelChunk* getChunk(int chunkX, int chunkZ);

	// Get block at world coordinates - simplified version
	VoxelType* getBlockAt(glm::vec3 worldPos);

	// Cleanup all chunks
	void cleanup();

	// Set world parameters
	void setRenderDistance(int distance) { renderDistance = distance; }
	int getRenderDistance() const { return renderDistance; }

	// NEW: World-level terrain generation functions
	float getTerrainHeight(float worldX, float worldZ);
	VoxelType getBlockType(float worldX, float worldY, float worldZ, float terrainHeight);

private:
	// Chunk storage using chunk coordinates as key
	std::unordered_map<long long, std::unique_ptr<VoxelChunk>> chunks;

	// World parameters
	int renderDistance;
	unsigned int worldSeed;

	// NEW: World-level noise generator for consistent terrain
	PerlinNoise worldNoise;

	// Current player position for chunk management
	glm::vec3 lastPlayerPos;

	// Helper function to convert chunk coordinates to a unique key
	long long getChunkKey(int chunkX, int chunkZ);

	// Convert world coordinates to chunk coordinates
	void getChunkCoords(glm::vec3 worldPos, int& chunkX, int& chunkZ);

	// Generate chunks around a position
	void generateChunksAroundPosition(glm::vec3 pos);

	// NEW: Generate terrain for a specific chunk using world-level noise
	void generateChunkTerrain(VoxelChunk* chunk, int chunkX, int chunkZ);

	// Remove chunks that are too far from player
	void unloadDistantChunks(glm::vec3 playerPos);

	// Check if a chunk should be loaded based on distance
	bool shouldLoadChunk(int chunkX, int chunkZ, int playerChunkX, int playerChunkZ, int maxDistance = -1);
};

#endif