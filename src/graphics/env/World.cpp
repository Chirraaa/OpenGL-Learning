#include "World.h"
#include "../models/voxelchunk.hpp"  // Include the full definition here
#include "../Shader.h"
#include <iostream>
#include <vector>
#include <cmath>

World::World(int renderDist, unsigned int seed)
    : renderDistance(renderDist), worldSeed(seed), lastPlayerPos(0.0f) {
    std::cout << "Created world with render distance: " << renderDistance << std::endl;
}

World::~World() {
    cleanup();
}

long long World::getChunkKey(int chunkX, int chunkZ) {
    // Combine chunk coordinates into a single key
    // Use bit shifting to avoid collisions
    return (static_cast<long long>(chunkX) << 32) | (static_cast<long long>(chunkZ) & 0xFFFFFFFF);
}

void World::getChunkCoords(glm::vec3 worldPos, int& chunkX, int& chunkZ) {
    // Assuming each chunk is 16x16 blocks (CHUNK_SIZE from VoxelChunk)
    const int CHUNK_SIZE = 16;

    // Convert world position to chunk coordinates
    chunkX = static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE));
    chunkZ = static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE));
}

void World::update(glm::vec3 playerPos) {
    // Debug output
    static bool firstUpdate = true;
    if (firstUpdate) {
        std::cout << "First world update at player position: ("
            << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << ")" << std::endl;
        firstUpdate = false;
    }

    // Check if player moved far enough to require chunk updates
    float distanceMoved = glm::length(playerPos - lastPlayerPos);
    if (distanceMoved > 8.0f || glm::length(lastPlayerPos) == 0.0f) { // Also update on first call
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

    // Generate chunks in a square around the player
    for (int x = playerChunkX - renderDistance; x <= playerChunkX + renderDistance; x++) {
        for (int z = playerChunkZ - renderDistance; z <= playerChunkZ + renderDistance; z++) {
            // Only generate chunks within circular render distance
            if (shouldLoadChunk(x, z, playerChunkX, playerChunkZ)) {
                long long key = getChunkKey(x, z);

                // Check if chunk already exists
                if (chunks.find(key) == chunks.end()) {
                    // Create new chunk at world position
                    glm::vec3 chunkWorldPos(x * 16.0f, 0.0f, z * 16.0f);

                    // Create chunk with world seed modified by chunk coordinates for variation
                    unsigned int chunkSeed = worldSeed + x * 1000 + z;
                    auto newChunk = std::make_unique<VoxelChunk>(chunkWorldPos, chunkSeed);

                    // Generate terrain for the chunk
                    newChunk->generateTerrain();

                    chunks[key] = std::move(newChunk);
                    std::cout << "Generated chunk at chunk coords (" << x << ", " << z
                        << ") world pos (" << chunkWorldPos.x << ", " << chunkWorldPos.y << ", " << chunkWorldPos.z << ")" << std::endl;
                }
            }
        }
    }
}

void World::unloadDistantChunks(glm::vec3 playerPos) {
    int playerChunkX, playerChunkZ;
    getChunkCoords(playerPos, playerChunkX, playerChunkZ);

    std::vector<long long> chunksToRemove;

    for (const auto& pair : chunks) {
        long long key = pair.first;

        // Extract chunk coordinates from key
        int chunkX = static_cast<int>(key >> 32);
        int chunkZ = static_cast<int>(key & 0xFFFFFFFF);

        // Check if chunk is too far away (add buffer to prevent constant loading/unloading)
        if (!shouldLoadChunk(chunkX, chunkZ, playerChunkX, playerChunkZ, renderDistance + 2)) {
            chunksToRemove.push_back(key);
        }
    }

    // Remove distant chunks
    for (long long key : chunksToRemove) {
        int chunkX = static_cast<int>(key >> 32);
        int chunkZ = static_cast<int>(key & 0xFFFFFFFF);

        chunks[key]->cleanup(); // Clean up GPU resources
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
    // Debug output - only print occasionally
    static int renderCallCount = 0;
    renderCallCount++;

    if (renderCallCount % 60 == 0) { // Every 60 frames
        std::cout << "Rendering " << chunks.size() << " chunks" << std::endl;
    }

    // Render all loaded chunks
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

    // Calculate local coordinates within the chunk
    const int CHUNK_SIZE = 16;
    int localX = static_cast<int>(worldPos.x) - (chunkX * CHUNK_SIZE);
    int localZ = static_cast<int>(worldPos.z) - (chunkZ * CHUNK_SIZE);
    int localY = static_cast<int>(worldPos.y);

    // Make sure coordinates are within chunk bounds
    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    if (localX >= 0 && localX < CHUNK_SIZE &&
        localZ >= 0 && localZ < CHUNK_SIZE &&
        localY >= 0 && localY < 64 && // CHUNK_HEIGHT
        chunk->hasVoxel(localX, localY, localZ)) {

        // This would need to be implemented in VoxelChunk to return a pointer
        // For now, return nullptr - you'd need to modify VoxelChunk class
        return nullptr;
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