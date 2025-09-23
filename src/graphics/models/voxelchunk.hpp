#ifndef VOXELCHUNK_HPP
#define VOXELCHUNK_HPP

#include "voxel.hpp"
#include "../../generation/perlin.h"

class VoxelChunk : public Voxel {
private:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_HEIGHT = 64;
    std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, bool>>> voxels;
    glm::vec3 chunkPosition;
    Mesh chunkMesh;
    bool needsRebuild = true;
    PerlinNoise perlinNoise;

public:
    VoxelChunk(glm::vec3 pos, unsigned int seed = 12345)
        : chunkPosition(pos), perlinNoise(seed) {
    }

    // Generate terrain using Perlin noise
    void generateTerrain() {
        voxels.clear(); // Clear existing voxels

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                // Calculate world coordinates for noise sampling
                double worldX = chunkPosition.x + x;
                double worldZ = chunkPosition.z + z;

                // Generate height using fractal Perlin noise
                // Scale controls the "zoom" level - smaller = more zoomed out
                double noiseValue = perlinNoise.fractalNoise(
                    worldX * 0.01,  // Scale factor
                    worldZ * 0.01,  // Scale factor
                    4,              // Octaves (detail layers)
                    0.5,            // Persistence (how much each octave contributes)
                    1.0             // Initial frequency
                );

                // Convert noise (-1 to 1) to height (0 to max height)
                int baseHeight = 32; // Base terrain level
                int variation = 16;  // How much the terrain can vary
                int height = baseHeight + (int)(noiseValue * variation);

                // Clamp height to valid range
                height = std::max(0, std::min(height, CHUNK_HEIGHT - 1));

                // Fill from bottom up to calculated height
                for (int y = 0; y <= height; y++) {
                    addVoxel(x, y, z);
                }
            }
        }

        needsRebuild = true;
    }

    // Different terrain generation methods you can experiment with:

    // Simple hills
    void generateSimpleHills() {
        voxels.clear();

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                double worldX = chunkPosition.x + x;
                double worldZ = chunkPosition.z + z;

                // Single octave for smooth hills
                double noiseValue = perlinNoise.noise(worldX * 0.05, worldZ * 0.05);
                int height = 20 + (int)(noiseValue * 10);
                height = std::max(0, std::min(height, CHUNK_HEIGHT - 1));

                for (int y = 0; y <= height; y++) {
                    addVoxel(x, y, z);
                }
            }
        }
        needsRebuild = true;
    }

    // Mountainous terrain
    void generateMountains() {
        voxels.clear();

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                double worldX = chunkPosition.x + x;
                double worldZ = chunkPosition.z + z;

                // Multiple scales for complexity
                double largeFeatures = perlinNoise.noise(worldX * 0.003, worldZ * 0.003) * 30;
                double mediumFeatures = perlinNoise.noise(worldX * 0.01, worldZ * 0.01) * 15;
                double smallFeatures = perlinNoise.noise(worldX * 0.05, worldZ * 0.05) * 5;

                int height = 25 + (int)(largeFeatures + mediumFeatures + smallFeatures);
                height = std::max(0, std::min(height, CHUNK_HEIGHT - 1));

                for (int y = 0; y <= height; y++) {
                    addVoxel(x, y, z);
                }
            }
        }
        needsRebuild = true;
    }

    // Caves and overhangs (3D noise)
    void generateCaves() {
        voxels.clear();

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                double worldX = chunkPosition.x + x;
                double worldZ = chunkPosition.z + z;

                // Base height
                double surfaceNoise = perlinNoise.noise(worldX * 0.01, worldZ * 0.01);
                int surfaceHeight = 30 + (int)(surfaceNoise * 15);

                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    double worldY = chunkPosition.y + y;

                    // 3D noise for caves (you'd need to extend PerlinNoise for 3D)
                    // For now, create caves below surface
                    if (y <= surfaceHeight) {
                        // Simple cave system using 2D noise at different Y levels
                        double caveNoise = perlinNoise.noise(worldX * 0.02, (worldZ + y * 10) * 0.02);

                        // Only add voxel if cave noise is above threshold
                        if (caveNoise > -0.3) { // Adjust threshold for cave size
                            addVoxel(x, y, z);
                        }
                    }
                }
            }
        }
        needsRebuild = true;
    }

    // Hash function for voxel positions
    int hashPos(int x, int y, int z) const {
        return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
    }

    // Add a voxel at local coordinates
    void addVoxel(int x, int y, int z) {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_HEIGHT && z >= 0 && z < CHUNK_SIZE) {
            voxels[x][y][z] = true;
            needsRebuild = true;
        }
    }

    // Remove a voxel
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

    // Check if voxel exists
    bool hasVoxel(int x, int y, int z) const {
        return voxels.count(x) && voxels.at(x).count(y) && voxels.at(x).at(y).count(z);
    }

    // Rebuild chunk mesh with face culling
    void rebuildMesh() {
        if (!needsRebuild) return;

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (const auto& x_pair : voxels) {
            int x = x_pair.first;
            for (const auto& y_pair : x_pair.second) {
                int y = y_pair.first;
                for (const auto& z_pair : y_pair.second) {
                    int z = z_pair.first;

                    // Use local coordinates (relative to chunk)
                    glm::vec3 localVoxelPos = glm::vec3(x, y, z);

                    // Only render faces that are exposed (not adjacent to another voxel)
                    if (!hasVoxel(x, y, z + 1)) addVoxelFace(vertices, indices, localVoxelPos, Face::FRONT);
                    if (!hasVoxel(x, y, z - 1)) addVoxelFace(vertices, indices, localVoxelPos, Face::BACK);
                    if (!hasVoxel(x - 1, y, z)) addVoxelFace(vertices, indices, localVoxelPos, Face::LEFT);
                    if (!hasVoxel(x + 1, y, z)) addVoxelFace(vertices, indices, localVoxelPos, Face::RIGHT);
                    if (!hasVoxel(x, y + 1, z)) addVoxelFace(vertices, indices, localVoxelPos, Face::TOP);
                    if (!hasVoxel(x, y - 1, z)) addVoxelFace(vertices, indices, localVoxelPos, Face::BOTTOM);
                }
            }
        }

        if (!vertices.empty()) {
            chunkMesh = Mesh(vertices, indices);

            // Add texture
            Texture dirtTexture("assets/textures", "dirt.png", aiTextureType_DIFFUSE);
            dirtTexture.load();
            chunkMesh.textures.push_back(dirtTexture);
            chunkMesh.setUseTexture(true);
        }

        needsRebuild = false;
    }

    void render(Shader shader) {
        rebuildMesh();

        if (chunkMesh.vertices.empty()) {
            return; // Nothing to render
        }

        // Set up model matrix for the chunk position
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, chunkPosition);
        shader.setMat4("model", model);

        // Set material properties that the shader expects
        shader.setFloat("material.shininess", 32.0f);

        chunkMesh.render(shader);
    }

    void cleanup() {
        chunkMesh.clearnup();
    }

private:
    void addVoxelFace(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
        glm::vec3 localPos, Face face) {
        unsigned int startIndex = vertices.size();

        // Face vertex templates (local coordinates)
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

        // Add vertices
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

        // Add indices for two triangles
        indices.insert(indices.end(), {
            startIndex, startIndex + 1, startIndex + 2,
            startIndex + 2, startIndex + 3, startIndex
            });
    }
};

#endif