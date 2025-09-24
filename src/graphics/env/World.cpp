#include "World.h"
#include "../models/voxelchunk.hpp"
#include "../Shader.h"
#include <iostream>
#include <vector>
#include <cmath>

World::World(int renderDist, unsigned int seed)
    : renderDistance(renderDist), worldSeed(seed), lastPlayerPos(0.0f),
    worldNoise(seed) { // Initialize world-level noise generator
    std::cout << "Created world with render distance: " << renderDistance << std::endl;
}

World::~World() {
    cleanup();
}

long long World::getChunkKey(int chunkX, int chunkZ) {
    return (static_cast<long long>(chunkX) << 32) | (static_cast<long long>(chunkZ) & 0xFFFFFFFF);
}

void World::getChunkCoords(glm::vec3 worldPos, int& chunkX, int& chunkZ) {
    const int CHUNK_SIZE = 16;
    chunkX = static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE));
    chunkZ = static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE));
}

// NEW: World-level terrain generation function
float World::getTerrainHeight(float worldX, float worldZ) {
    // Use consistent noise sampling across the entire world
    double noiseValue = worldNoise.fractalNoise(
        worldX * 0.01,   // Scale factor
        worldZ * 0.01,
        4,               // Octaves
        0.5,             // Persistence
        1.0              // Lacunarity
    );

    int baseHeight = 32;
    int variation = 16;
    int height = baseHeight + (int)(noiseValue * variation);
    return std::max(0.0f, std::min((float)height, 63.0f)); // Clamp to chunk height
}

// NEW: World-level biome/block type determination
VoxelType World::getBlockType(float worldX, float worldY, float worldZ, float terrainHeight) {
    int seaLevel = 34;
    int y = (int)worldY;

    if (terrainHeight < seaLevel) {
        // Beach/underwater biome
        if (y <= terrainHeight) {
            return VoxelType::SAND;
        }
    }
    else {
        // Land biome
        if (y == (int)terrainHeight) {
            return VoxelType::GRASS; // Top layer
        }
        else if (y >= (int)terrainHeight - 3) {
            return VoxelType::DIRT;  // Subsurface
        }
        else if (y <= (int)terrainHeight) {
            return VoxelType::COBBLESTONE; // Deep layers
        }
    }

    return VoxelType::DIRT; // Fallback
}

void World::update(glm::vec3 playerPos) {
    static bool firstUpdate = true;
    if (firstUpdate) {
        std::cout << "First world update at player position: ("
            << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << ")" << std::endl;
        firstUpdate = false;
    }

    float distanceMoved = glm::length(playerPos - lastPlayerPos);
    if (distanceMoved > 8.0f || glm::length(lastPlayerPos) == 0.0f) {
        std::cout << "Updating chunks - distance moved: " << distanceMoved << std::endl;
        generateChunksAroundPosition(playerPos);
        unloadDistantChunks(playerPos);
        lastPlayerPos = playerPos;
        std::cout << "Total chunks loaded: " << chunks.size() << std::endl;
    }
}

void World::generateChunksAroundPosition(glm::vec3 pos) {
    int playerChunkX, playerChunkZ;
    getChunkCoords(pos, playerChunkX, playerChunkZ);

    std::cout << "Player at chunk coordinates: (" << playerChunkX << ", " << playerChunkZ << ")" << std::endl;

    for (int x = playerChunkX - renderDistance; x <= playerChunkX + renderDistance; x++) {
        for (int z = playerChunkZ - renderDistance; z <= playerChunkZ + renderDistance; z++) {
            if (shouldLoadChunk(x, z, playerChunkX, playerChunkZ)) {
                long long key = getChunkKey(x, z);

                if (chunks.find(key) == chunks.end()) {
                    glm::vec3 chunkWorldPos(x * 16.0f, 0.0f, z * 16.0f);

                    // Pass the world instance to chunk for homogeneous generation
                    auto newChunk = std::make_unique<VoxelChunk>(chunkWorldPos, worldSeed);

                    // Generate terrain using world-level functions
                    generateChunkTerrain(newChunk.get(), x, z);

                    chunks[key] = std::move(newChunk);
                    std::cout << "Generated chunk at chunk coords (" << x << ", " << z
                        << ") world pos (" << chunkWorldPos.x << ", " << chunkWorldPos.y << ", " << chunkWorldPos.z << ")" << std::endl;
                }
            }
        }
    }
}

// NEW: Generate terrain for a specific chunk using world-level noise
void World::generateChunkTerrain(VoxelChunk* chunk, int chunkX, int chunkZ) {
    const int CHUNK_SIZE = 16;
    const int CHUNK_HEIGHT = 64;

    // Clear any existing voxels
    chunk->clearVoxels();

    for (int localX = 0; localX < CHUNK_SIZE; localX++) {
        for (int localZ = 0; localZ < CHUNK_SIZE; localZ++) {
            // Convert to world coordinates
            float worldX = chunkX * CHUNK_SIZE + localX;
            float worldZ = chunkZ * CHUNK_SIZE + localZ;

            // Get terrain height using world-level noise
            float terrainHeight = getTerrainHeight(worldX, worldZ);

            // Generate vertical column
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                if (y <= terrainHeight) {
                    VoxelType blockType = getBlockType(worldX, y, worldZ, terrainHeight);
                    chunk->addVoxel(localX, y, localZ, blockType);
                }
            }
        }
    }

    chunk->markForRebuild();
}

void World::unloadDistantChunks(glm::vec3 playerPos) {
    int playerChunkX, playerChunkZ;
    getChunkCoords(playerPos, playerChunkX, playerChunkZ);

    std::vector<long long> chunksToRemove;

    for (const auto& pair : chunks) {
        long long key = pair.first;
        int chunkX = static_cast<int>(key >> 32);
        int chunkZ = static_cast<int>(key & 0xFFFFFFFF);

        if (!shouldLoadChunk(chunkX, chunkZ, playerChunkX, playerChunkZ, renderDistance + 2)) {
            chunksToRemove.push_back(key);
        }
    }

    for (long long key : chunksToRemove) {
        int chunkX = static_cast<int>(key >> 32);
        int chunkZ = static_cast<int>(key & 0xFFFFFFFF);

        chunks[key]->cleanup();
        chunks.erase(key);
        std::cout << "Unloaded chunk at (" << chunkX << ", " << chunkZ << ")" << std::endl;
    }
}

bool World::shouldLoadChunk(int chunkX, int chunkZ, int playerChunkX, int playerChunkZ, int maxDistance) {
    if (maxDistance == -1) maxDistance = renderDistance;

    int dx = chunkX - playerChunkX;
    int dz = chunkZ - playerChunkZ;
    float distance = std::sqrt(dx * dx + dz * dz);

    return distance <= maxDistance;
}

void World::render(Shader& shader) {
    static int renderCallCount = 0;
    renderCallCount++;

    if (renderCallCount % 60 == 0) {
        std::cout << "Rendering " << chunks.size() << " chunks" << std::endl;
    }

    int chunksRendered = 0;
    for (const auto& pair : chunks) {
        VoxelChunk* chunk = pair.second.get();
        if (chunk) {
            chunk->render(shader);
            chunksRendered++;
        }
    }

    if (renderCallCount % 60 == 0 && chunksRendered > 0) {
        std::cout << "Actually rendered " << chunksRendered << " chunks" << std::endl;
    }
}

VoxelChunk* World::getChunk(int chunkX, int chunkZ) {
    long long key = getChunkKey(chunkX, chunkZ);
    auto it = chunks.find(key);

    if (it != chunks.end()) {
        return it->second.get();
    }

    return nullptr;
}

VoxelType* World::getBlockAt(glm::vec3 worldPos) {
    int chunkX, chunkZ;
    getChunkCoords(worldPos, chunkX, chunkZ);

    VoxelChunk* chunk = getChunk(chunkX, chunkZ);
    if (!chunk) return nullptr;

    const int CHUNK_SIZE = 16;
    int localX = static_cast<int>(worldPos.x) - (chunkX * CHUNK_SIZE);
    int localZ = static_cast<int>(worldPos.z) - (chunkZ * CHUNK_SIZE);
    int localY = static_cast<int>(worldPos.y);

    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    if (localX >= 0 && localX < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE &&
        localY >= 0 && localY < 64 &&
        chunk->hasVoxel(localX, localY, localZ)) {

        return nullptr; // You'd need to modify VoxelChunk to return the actual type
    }

    return nullptr;
}

void World::cleanup() {
    std::cout << "Cleaning up " << chunks.size() << " chunks" << std::endl;
    for (auto& pair : chunks) {
        pair.second->cleanup();
    }
    chunks.clear();
    std::cout << "World cleanup complete" << std::endl;
}