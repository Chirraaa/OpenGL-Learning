#include "World.h"
#include "../models/voxelchunk.hpp"
#include "../Shader.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <thread>

World::World(int renderDist, unsigned int seed)
	: renderDistance(renderDist),
	worldSeed(seed),
	lastPlayerPos(0.0f),
	worldNoise(seed),
	m_isRunning(true) {
	std::cout << "Created world with render distance: " << renderDistance << std::endl;

	unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
	std::cout << "Starting " << numThreads << " chunk worker threads." << std::endl;
	for (unsigned int i = 0; i < numThreads; ++i) {
		m_chunkWorkers.emplace_back(&World::chunkWorkerLoop, this);
	}
}

World::~World() {
	std::cout << "Stopping worker threads..." << std::endl;
	m_isRunning = false;
	m_chunksToLoadQueue.stop();
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

void World::update(glm::vec3 playerPos) {
	float distanceMoved = glm::length(playerPos - lastPlayerPos);
	if (distanceMoved > 8.0f || glm::length(lastPlayerPos) == 0.0f) {
		generateChunksAroundPosition(playerPos);
		unloadDistantChunks(playerPos);
		lastPlayerPos = playerPos;
	}

	int uploadsThisFrame = 0;
	const int maxUploadsPerFrame = 2;
	ChunkMeshData meshData;
	while (uploadsThisFrame < maxUploadsPerFrame && m_meshesToUploadQueue.try_pop(meshData)) {
		long long key = meshData.chunkKey;

		auto newChunk = std::make_unique<VoxelChunk>(meshData.chunkPosition, worldSeed);

		// This now works because both are unordered_maps
		newChunk->voxels = std::move(meshData.voxels);

		newChunk->uploadMesh(meshData);
		chunks[key] = std::move(newChunk);

		{
			std::lock_guard<std::mutex> lock(m_worldMutex);
			m_generatingChunks.erase(key);
		}

		uploadsThisFrame++;
	}
}

void World::setBlock(int worldX, int worldY, int worldZ, VoxelType type) {
	int chunkX, chunkZ;
	getChunkCoords(glm::vec3(worldX, worldY, worldZ), chunkX, chunkZ);
	VoxelChunk* chunk = getChunk(chunkX, chunkZ);

	if (chunk) {
		int localX = worldX - (chunkX * CHUNK_SIZE);
		int localZ = worldZ - (chunkZ * CHUNK_SIZE);

		chunk->setBlock(localX, worldY, localZ, type);
		chunk->rebuildMesh();
	}
}

VoxelType World::getBlockTypeAt(int worldX, int worldY, int worldZ) {
	int chunkX, chunkZ;
	getChunkCoords(glm::vec3(worldX, worldY, worldZ), chunkX, chunkZ);
	VoxelChunk* chunk = getChunk(chunkX, chunkZ);

	if (chunk) {
		int localX = worldX - (chunkX * CHUNK_SIZE);
		int localZ = worldZ - (chunkZ * CHUNK_SIZE);
		// CORRECTED: Call the renamed function getBlockType
		return chunk->getBlockType(localX, worldY, localZ);
	}
	return VoxelType::AIR;
}

void World::generateChunksAroundPosition(glm::vec3 pos) {
	int playerChunkX, playerChunkZ;
	getChunkCoords(pos, playerChunkX, playerChunkZ);

	for (int x = playerChunkX - renderDistance; x <= playerChunkX + renderDistance; x++) {
		for (int z = playerChunkZ - renderDistance; z <= playerChunkZ + renderDistance; z++) {
			if (shouldLoadChunk(x, z, playerChunkX, playerChunkZ)) {
				long long key = getChunkKey(x, z);

				std::lock_guard<std::mutex> lock(m_worldMutex);
				if (chunks.find(key) == chunks.end() && m_generatingChunks.find(key) == m_generatingChunks.end()) {
					m_generatingChunks.insert(key);
					m_chunksToLoadQueue.push(glm::ivec2(x, z));
				}
			}
		}
	}
}

void World::chunkWorkerLoop() {
	while (m_isRunning) {
		glm::ivec2 chunkCoords;
		if (!m_chunksToLoadQueue.wait_and_pop(chunkCoords)) {
			continue;
		}

		int chunkX = chunkCoords.x;
		int chunkZ = chunkCoords.y;

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

// CORRECTED: Removed the extra, conflicting getBlock implementation.

void World::cleanup() {
	std::cout << "Cleaning up " << chunks.size() << " chunks" << std::endl;
	chunks.clear();
	std::cout << "World cleanup complete" << std::endl;
}