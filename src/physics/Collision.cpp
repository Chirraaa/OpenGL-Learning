#include "Collision.h"
#include "../graphics/env/World.h" // You'll need to include your World class
#include "../graphics/models/voxelchunk.hpp" // And your VoxelChunk class

CollisionInfo CollisionDetector::checkWorldCollision(const AABB& aabb, World* world) {
    CollisionInfo result;

    // Calculate which chunks this AABB might intersect
    int minChunkX = static_cast<int>(floor(aabb.min.x / 16.0f));
    int maxChunkX = static_cast<int>(floor(aabb.max.x / 16.0f));
    int minChunkZ = static_cast<int>(floor(aabb.min.z / 16.0f));
    int maxChunkZ = static_cast<int>(floor(aabb.max.z / 16.0f));

    // Check collision with each potentially intersecting chunk
    for (int chunkX = minChunkX; chunkX <= maxChunkX; chunkX++) {
        for (int chunkZ = minChunkZ; chunkZ <= maxChunkZ; chunkZ++) {
            // Get chunk from world (you'll need to implement this method in World)
            VoxelChunk* chunk = world->getChunk(chunkX, chunkZ);
            if (chunk) {
                CollisionInfo chunkCollision = checkChunkCollision(aabb, chunk);
                if (chunkCollision.hasCollision) {
                    // If we find a collision, return the first one
                    // In a more advanced system, you might want to find the closest collision
                    result = chunkCollision;
                    break;
                }
            }
        }
        if (result.hasCollision) break;
    }

    return result;
}

CollisionInfo CollisionDetector::checkChunkCollision(const AABB& aabb, VoxelChunk* chunk) {
    CollisionInfo result;

    // Convert world coordinates to chunk-local coordinates
    glm::vec3 chunkPos = chunk->getPosition(); // Assuming you have this method

    // Calculate which voxels in the chunk might intersect
    int minX = static_cast<int>(floor(aabb.min.x - chunkPos.x));
    int maxX = static_cast<int>(ceil(aabb.max.x - chunkPos.x));
    int minY = static_cast<int>(floor(aabb.min.y - chunkPos.y));
    int maxY = static_cast<int>(ceil(aabb.max.y - chunkPos.y));
    int minZ = static_cast<int>(floor(aabb.min.z - chunkPos.z));
    int maxZ = static_cast<int>(ceil(aabb.max.z - chunkPos.z));

    // Clamp to chunk bounds (assuming 16x64x16 chunks)
    minX = std::max(0, std::min(15, minX));
    maxX = std::max(0, std::min(15, maxX));
    minY = std::max(0, std::min(63, minY)); // Adjust based on your chunk height
    maxY = std::max(0, std::min(63, maxY));
    minZ = std::max(0, std::min(15, minZ));
    maxZ = std::max(0, std::min(15, maxZ));

    float closestDistance = FLT_MAX;

    // Check each voxel in the range
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (isVoxelSolid(x, y, z, chunk)) {
                    // Create AABB for this voxel in world coordinates
                    AABB voxelAABB = getVoxelAABB(
                        static_cast<int>(chunkPos.x) + x,
                        static_cast<int>(chunkPos.y) + y,
                        static_cast<int>(chunkPos.z) + z
                    );

                    CollisionInfo voxelCollision = checkAABBCollision(aabb, voxelAABB);
                    if (voxelCollision.hasCollision) {
                        if (voxelCollision.penetration < closestDistance) {
                            closestDistance = voxelCollision.penetration;
                            result = voxelCollision;
                        }
                    }
                }
            }
        }
    }

    return result;
}

bool CollisionDetector::isVoxelSolid(int x, int y, int z, VoxelChunk* chunk) {
    // You'll need to implement this based on your voxel storage system
    // This assumes you have a method to check if a voxel is solid
    // Return true if the voxel at local coordinates (x,y,z) is solid

    // Example implementation (you'll need to adapt this):
    if (x < 0 || x >= 16 || y < 0 || y >= 64 || z < 0 || z >= 16) {
        return false; // Out of bounds
    }

    // Assuming you have a voxel array or similar storage
    // return chunk->getVoxel(x, y, z) != VoxelType::AIR;

    // Placeholder - replace with your actual voxel checking logic
    return true; // For now, assume all voxels are solid
}

AABB CollisionDetector::getVoxelAABB(int x, int y, int z) {
    glm::vec3 center(static_cast<float>(x) + 0.5f,
        static_cast<float>(y) + 0.5f,
        static_cast<float>(z) + 0.5f);
    glm::vec3 size(1.0f, 1.0f, 1.0f); // Standard voxel size
    return AABB(center, size);
}

void CollisionDetector::resolveCollision(RigidBody& rb, const CollisionInfo& collision, float objectRadius) {
    if (!collision.hasCollision) return;

    // Move object out of collision along the collision normal
    rb.pos += collision.normal * collision.penetration;

    // Apply collision response to velocity
    // Remove velocity component in the direction of the collision normal
    float velocityInNormal = glm::dot(rb.velocity, collision.normal);
    if (velocityInNormal < 0) { // Only if moving into the collision
        rb.velocity -= collision.normal * velocityInNormal;

        // Optional: Add some damping/friction
        rb.velocity *= 0.9f;
    }
}

CollisionInfo CollisionDetector::sweptAABB(const AABB& aabb, glm::vec3 velocity, float dt, World* world) {
    // This is a simplified swept collision detection
    // For a more robust implementation, you might want to use a more sophisticated algorithm

    CollisionInfo result;

    glm::vec3 displacement = velocity * dt;

    // If no movement, just do static collision check
    if (glm::length(displacement) < 0.001f) {
        return checkWorldCollision(aabb, world);
    }

    // Create expanded AABB that encompasses the entire movement
    AABB expandedAABB;
    expandedAABB.min = glm::min(aabb.min, aabb.min + displacement);
    expandedAABB.max = glm::max(aabb.max, aabb.max + displacement);

    // Check for potential collisions in the expanded area
    CollisionInfo staticCollision = checkWorldCollision(expandedAABB, world);

    if (staticCollision.hasCollision) {
        // Perform binary search to find the exact collision point
        float t = 0.0f;
        float tMax = 1.0f;
        const int maxIterations = 10;

        for (int i = 0; i < maxIterations; i++) {
            float tMid = (t + tMax) * 0.5f;
            AABB testAABB = aabb;
            testAABB.min += displacement * tMid;
            testAABB.max += displacement * tMid;

            CollisionInfo testCollision = checkWorldCollision(testAABB, world);
            if (testCollision.hasCollision) {
                tMax = tMid;
                result = testCollision;
            }
            else {
                t = tMid;
            }
        }

        result.hasCollision = true;
    }

    return result;
}

CollisionInfo CollisionDetector::checkAABBCollision(const AABB& a, const AABB& b) {
    CollisionInfo result;

    // Check if AABBs intersect
    if (!a.intersects(b)) {
        return result; // No collision
    }

    result.hasCollision = true;

    // Calculate overlap on each axis
    float overlapX = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    float overlapY = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    float overlapZ = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    // Find the axis with minimum overlap (this is the separation axis)
    if (overlapX <= overlapY && overlapX <= overlapZ) {
        // X axis has minimum overlap
        result.penetration = overlapX;
        result.normal = (a.getCenter().x < b.getCenter().x) ? glm::vec3(-1, 0, 0) : glm::vec3(1, 0, 0);
    }
    else if (overlapY <= overlapZ) {
        // Y axis has minimum overlap
        result.penetration = overlapY;
        result.normal = (a.getCenter().y < b.getCenter().y) ? glm::vec3(0, -1, 0) : glm::vec3(0, 1, 0);
    }
    else {
        // Z axis has minimum overlap
        result.penetration = overlapZ;
        result.normal = (a.getCenter().z < b.getCenter().z) ? glm::vec3(0, 0, -1) : glm::vec3(0, 0, 1);
    }

    return result;
}

glm::vec3 CollisionDetector::closestPointOnAABB(const glm::vec3& point, const AABB& aabb) {
    return glm::clamp(point, aabb.min, aabb.max);
}