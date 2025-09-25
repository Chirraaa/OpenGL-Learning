#ifndef VOXELCHUNK_HPP
#define VOXELCHUNK_HPP

#include "voxel.hpp"
#include "../../generation/perlin.h"
#include <map>
#include <vector>

// Forward declare the struct to avoid circular dependency
struct ChunkMeshData;

enum class VoxelType {
    DIRT = 0,
    COBBLESTONE = 1,
    SAND = 2,
    GRASS = 3
};

struct VoxelTextures {
    Texture diffuse;
    Texture top;
    Texture side;
    Texture bottom;
    bool hasMultipleTextures = false;
};

class VoxelChunk { // MODIFIED: No longer inherits from Voxel
private:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_HEIGHT = 64;

    glm::vec3 chunkPosition;
    std::map<VoxelType, std::vector<Mesh>> chunkMeshes;
    std::map<VoxelType, VoxelTextures> voxelTextures;
    bool texturesLoaded = false;

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

    // NEW: This helper function is now static so worker threads can use it without a VoxelChunk instance.
    static void addFaceToMeshData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, glm::vec3 localPos, Face face);

private:
    void loadTextures();
};

#endif