#include "World.h"
#include "../models/voxelchunk.hpp"
#include "../Shader.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>

// MODIFIED: Constructor now starts the worker threads.
World::World(int renderDist, unsigned int seed)
	: renderDistance(renderDist),
	worldSeed(seed),
	lastPlayerPos(0.0f),
	worldNoise(seed),
	m_isRunning(true) {
	std::cout << "Created world with render distance: " << renderDistance << std::endl;

	// NEW: Start worker threads. We use a number based on hardware concurrency, but at least 1.
	unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
	std::cout << "Starting " << numThreads << " chunk worker threads." << std::endl;
	for (unsigned int i = 0; i < numThreads; ++i) {
		m_chunkWorkers.emplace_back(&World::chunkWorkerLoop, this);
	}
}

// MODIFIED: Destructor now safely stops and joins the worker threads.
World::~World() {
	std::cout << "Stopping worker threads..." << std::endl;
	m_isRunning = false;
	m_chunksToLoadQueue.stop(); // Wake up any sleeping threads
	for (auto& worker : m_chunkWorkers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
	std::cout << "Worker threads stopped." << std::endl;
	cleanup();
}

long long World::getChunkKey(int chunkX, int chunkZ) {
	return (static_cast<long long>(chunkX) << 32) | (static_cast<long long>(chunkZ) & 0xFFFFFFFF);
}

void World::getChunkCoords(glm::vec3 worldPos, int& chunkX, int& chunkZ) {
	chunkX = static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE));
	chunkZ = static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE));
}

float World::getTerrainHeight(float worldX, float worldZ) {
	double noiseValue = worldNoise.fractalNoise(worldX * 0.01, worldZ * 0.01, 4, 0.5, 1.0);
	int baseHeight = CHUNK_HEIGHT;
	int variation = CHUNK_SIZE;
	int height = baseHeight + static_cast<int>(noiseValue * variation);
	return std::max(0.0f, std::min(static_cast<float>(height), 63.0f));
}


VoxelType World::getBlockType(float worldX, float worldY, float worldZ, float terrainHeight) {
	int seaLevel = 34;
	int y = static_cast<int>(worldY);

	// FIXED: Return AIR for blocks above terrain
	if (y > terrainHeight) return VoxelType::AIR;

	if (terrainHeight < seaLevel && y <= terrainHeight) {
		return VoxelType::SAND;
	}

	if (y == static_cast<int>(terrainHeight)) {
		return VoxelType::GRASS;
	}
	else if (y >= static_cast<int>(terrainHeight) - 3) {
		return VoxelType::DIRT;
	}
	else {
		return VoxelType::COBBLESTONE;
	}
}

// MODIFIED: Update now handles both queueing new chunks and uploading finished ones.
void World::update(glm::vec3 playerPos) {
	float distanceMoved = glm::length(playerPos - lastPlayerPos);
	if (distanceMoved > 8.0f || glm::length(lastPlayerPos) == 0.0f) {
		generateChunksAroundPosition(playerPos);
		unloadDistantChunks(playerPos);
		lastPlayerPos = playerPos;
	}

	// NEW: Process the queue of finished meshes from worker threads.
	// We limit the number of uploads per frame to prevent a single frame from
	// stuttering if many chunks finish at once.
	int uploadsThisFrame = 0;
	const int maxUploadsPerFrame = 2;
	ChunkMeshData meshData;
	while (uploadsThisFrame < maxUploadsPerFrame && m_meshesToUploadQueue.try_pop(meshData)) {
		long long key = meshData.chunkKey;

		// Create a new chunk and upload the mesh data to the GPU
		auto newChunk = std::make_unique<VoxelChunk>(meshData.chunkPosition, worldSeed);
		newChunk->uploadMesh(meshData); // This performs the OpenGL calls
		chunks[key] = std::move(newChunk);

		{ // Remove from the generating set
			std::lock_guard<std::mutex> lock(m_worldMutex);
			m_generatingChunks.erase(key);
		}

		uploadsThisFrame++;
	}
}

// MODIFIED: This function now just adds chunk coordinates to a queue.
void World::generateChunksAroundPosition(glm::vec3 pos) {
	int playerChunkX, playerChunkZ;
	getChunkCoords(pos, playerChunkX, playerChunkZ);

	for (int x = playerChunkX - renderDistance; x <= playerChunkX + renderDistance; x++) {
		for (int z = playerChunkZ - renderDistance; z <= playerChunkZ + renderDistance; z++) {
			if (shouldLoadChunk(x, z, playerChunkX, playerChunkZ)) {
				long long key = getChunkKey(x, z);

				std::lock_guard<std::mutex> lock(m_worldMutex);
				// Queue the chunk for loading if it doesn't exist and isn't already being generated.
				if (chunks.find(key) == chunks.end() && m_generatingChunks.find(key) == m_generatingChunks.end()) {
					m_generatingChunks.insert(key);
					m_chunksToLoadQueue.push(glm::ivec2(x, z));
				}
			}
		}
	}
}

// NEW: This is the heart of the asynchronous system. This function is run by each worker thread.
void World::chunkWorkerLoop() {
	while (m_isRunning) {
		glm::ivec2 chunkCoords;
		// Wait for a chunk to be available in the queue.
		if (!m_chunksToLoadQueue.wait_and_pop(chunkCoords)) {
			continue; // Queue was stopped
		}

		int chunkX = chunkCoords.x;
		int chunkZ = chunkCoords.y;

		// --- Step 1: Generate Voxel Data (CPU Intensive) ---
		ChunkMeshData meshData;
		meshData.chunkKey = getChunkKey(chunkX, chunkZ);
		meshData.chunkPosition = glm::vec3(chunkX * 16.0f, 0.0f, chunkZ * 16.0f);

		for (int localX = 0; localX < 16; localX++) {
			for (int localZ = 0; localZ < 16; localZ++) {
				float worldX = chunkX * 16 + localX;
				float worldZ = chunkZ * 16 + localZ;
				float terrainHeight = getTerrainHeight(worldX, worldZ);
				for (int y = 0; y < 64; y++) {
					if (y <= terrainHeight) {
						VoxelType blockType = getBlockType(worldX, y, worldZ, terrainHeight);
						meshData.voxels[localX][y][localZ] = blockType;
					}
				}
			}
		}

		// --- Step 2: Build Mesh Data (CPU Intensive) ---
		// This is the logic from the old VoxelChunk::rebuildMesh
		auto hasVoxel = [&](int x, int y, int z) {
			return meshData.voxels.count(x) && meshData.voxels.at(x).count(y) && meshData.voxels.at(x).at(y).count(z);
			};

		for (const auto& x_pair : meshData.voxels) {
			int x = x_pair.first;
			for (const auto& y_pair : x_pair.second) {
				int y = y_pair.first;
				for (const auto& z_pair : y_pair.second) {
					int z = z_pair.first;
					VoxelType currentType = z_pair.second;
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

		// --- Step 3: Push completed data to the main thread's queue ---
		m_meshesToUploadQueue.push(std::move(meshData));
	}
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
		chunks.erase(key);
	}
}

bool World::shouldLoadChunk(int chunkX, int chunkZ, int playerChunkX, int playerChunkZ, int maxDistance) {
	if (maxDistance == -1) maxDistance = renderDistance;
	int dx = chunkX - playerChunkX;
	int dz = chunkZ - playerChunkZ;
	return (dx * dx + dz * dz) <= (maxDistance * maxDistance);
}

void World::render(Shader& shader) {
	for (const auto& pair : chunks) {
		VoxelChunk* chunk = pair.second.get();
		if (chunk) {
			chunk->render(shader);
		}
	}
}

VoxelChunk* World::getChunk(int chunkX, int chunkZ) {
	long long key = getChunkKey(chunkX, chunkZ);
	auto it = chunks.find(key);
	return (it != chunks.end()) ? it->second.get() : nullptr;
}

Voxel* World::getBlock(int worldX, int worldY, int worldZ) {
	int chunkX, chunkZ;
	getChunkCoords(glm::vec3(worldX, worldY, worldZ), chunkX, chunkZ);

	int localX = worldX - (chunkX * CHUNK_SIZE);
	int localZ = worldZ - (chunkZ * CHUNK_SIZE);

	if (localX < 0) localX += CHUNK_SIZE;
	if (localZ < 0) localZ += CHUNK_SIZE;

	VoxelChunk* chunk = getChunk(chunkX, chunkZ);
	if (chunk && worldY >= 0 && worldY < CHUNK_HEIGHT) {
		return &chunk->getBlock(localX, worldY, localZ);
	}

	return nullptr;
}

void World::cleanup() {
	std::cout << "Cleaning up " << chunks.size() << " chunks" << std::endl;
	chunks.clear();
	std::cout << "World cleanup complete" << std::endl;
}