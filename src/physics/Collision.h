#ifndef COLLISION_H
#define COLLISION_H

#include <glm/glm.hpp>
#include <vector>
#include "RigidBody.h"

// Forward declarations
class VoxelChunk;
class World;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() = default;
    AABB(glm::vec3 center, glm::vec3 size) {
        min = center - size * 0.5f;
        max = center + size * 0.5f;
    }

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 getSize() const {
        return max - min;
    }
};

struct CollisionInfo {
    bool hasCollision = false;
    glm::vec3 normal = glm::vec3(0.0f);
    float penetration = 0.0f;
    glm::vec3 contactPoint = glm::vec3(0.0f);
};

class CollisionDetector {
public:
    // Check collision between AABB and world voxels
    static CollisionInfo checkWorldCollision(const AABB& aabb, World* world);

    // Check collision between AABB and a specific chunk
    static CollisionInfo checkChunkCollision(const AABB& aabb, VoxelChunk* chunk);

    // Check if a specific voxel position is solid
    static bool isVoxelSolid(int x, int y, int z, VoxelChunk* chunk);

    // Get AABB for a voxel at world position
    static AABB getVoxelAABB(int x, int y, int z);

    // Resolve collision by moving object out of collision
    static void resolveCollision(RigidBody& rb, const CollisionInfo& collision, float objectRadius = 0.5f);

    // Perform swept collision detection (continuous collision detection)
    static CollisionInfo sweptAABB(const AABB& aabb, glm::vec3 velocity, float dt, World* world);

private:
    // Helper function to check AABB vs AABB collision
    static CollisionInfo checkAABBCollision(const AABB& a, const AABB& b);

    // Get the closest point on AABB to a point
    static glm::vec3 closestPointOnAABB(const glm::vec3& point, const AABB& aabb);
};

// Physics component that can be added to objects for collision detection
class Collider {
public:
    AABB bounds;
    bool isTrigger = false;  // If true, detects collisions but doesn't resolve them

    Collider(glm::vec3 size = glm::vec3(1.0f)) : bounds(glm::vec3(0.0f), size) {}

    void updatePosition(glm::vec3 position) {
        glm::vec3 size = bounds.getSize();
        bounds.min = position - size * 0.5f;
        bounds.max = position + size * 0.5f;
    }

    CollisionInfo checkCollision(World* world) {
        return CollisionDetector::checkWorldCollision(bounds, world);
    }
};

#endif // COLLISION_H