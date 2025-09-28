#ifndef DONUT_HPP
#define DONUT_HPP
#include "../Model.h"
#include "modelarray.hpp"
#include "../../graphics/env/World.h"
#include <vector>
#include <cmath>

class Donut : public Model {
public:
    // Physics properties for collision detection
    float width = 1.0f;
    float height = 0.5f;
    float depth = 1.0f;
    bool onGround = false;
    float bounciness = 0.9f; // How much it bounces on collision

    // Reference to world for collision checking
    World* world = nullptr;

    Donut(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 size = glm::vec3(1.0f))
        : Model(pos, size, true) {
        angle = -130.0f;
        rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);

        // Set collision box size based on model size
        width = size.x*4;
        height = size.y*4;
        depth = size.z*4;
    };

    void init() {
        loadModel("assets/models/donut/scene.gltf");
    }

    void setWorld(World* worldPtr) {
        world = worldPtr;
    }

    void updatePhysics(float dt) {
        if (!world) return;

        // Store old position
        glm::vec3 oldPos = rb.pos;

        // Update physics normally first
        rb.update(dt);

        // Then check for collisions and resolve them
        resolveCollisions(rb.pos, oldPos);
    }

private:
    void resolveCollisions(glm::vec3& newPosition, glm::vec3 oldPosition) {
        // Test X movement
        glm::vec3 testPos = glm::vec3(newPosition.x, oldPosition.y, oldPosition.z);
        if (checkCollision(testPos)) {
            newPosition.x = oldPosition.x;
            rb.velocity.x = -rb.velocity.x * bounciness; // Bounce
        }

        // Test Y movement (gravity/falling)
        testPos = glm::vec3(newPosition.x, newPosition.y, oldPosition.z);
        if (checkCollision(testPos)) {
            if (rb.velocity.y < 0.0f) {
                // Falling - land on top of block
                newPosition.y = std::floor(newPosition.y + height) - height + 1.0f;
                onGround = true;
                rb.velocity.y = -rb.velocity.y * bounciness; // Bounce up
            }
            else if (rb.velocity.y > 0.0f) {
                // Rising - hit ceiling
                newPosition.y = std::floor(newPosition.y) - height;
                rb.velocity.y = -rb.velocity.y * bounciness; // Bounce down
            }
        }
        else {
            onGround = false;
        }

        // Test Z movement
        testPos = glm::vec3(newPosition.x, newPosition.y, newPosition.z);
        if (checkCollision(testPos)) {
            newPosition.z = oldPosition.z;
            rb.velocity.z = -rb.velocity.z * bounciness; // Bounce
        }
    }

    bool checkCollision(glm::vec3 position) {
        if (!world) return false;

        // Donut bounding box
        float minX = position.x - width / 2.0f;
        float maxX = position.x + width / 2.0f;
        float minY = position.y;
        float maxY = position.y + height;
        float minZ = position.z - depth / 2.0f;
        float maxZ = position.z + depth / 2.0f;

        // Check all blocks that might intersect with donut
        int minBlockX = static_cast<int>(std::floor(minX));
        int maxBlockX = static_cast<int>(std::floor(maxX));
        int minBlockY = static_cast<int>(std::floor(minY));
        int maxBlockY = static_cast<int>(std::floor(maxY));
        int minBlockZ = static_cast<int>(std::floor(minZ));
        int maxBlockZ = static_cast<int>(std::floor(maxZ));

        for (int blockX = minBlockX; blockX <= maxBlockX; blockX++) {
            for (int blockY = minBlockY; blockY <= maxBlockY; blockY++) {
                for (int blockZ = minBlockZ; blockZ <= maxBlockZ; blockZ++) {
                    if (isBlockSolid(blockX, blockY, blockZ)) {
                        // Block occupies space from (blockX, blockY, blockZ) to (blockX+1, blockY+1, blockZ+1)
                        float blockMinX = blockX;
                        float blockMaxX = blockX + 1.0f;
                        float blockMinY = blockY;
                        float blockMaxY = blockY + 1.0f;
                        float blockMinZ = blockZ;
                        float blockMaxZ = blockZ + 1.0f;

                        // Check if donut bounding box intersects with block bounding box
                        if (maxX > blockMinX && minX < blockMaxX &&
                            maxY > blockMinY && minY < blockMaxY &&
                            maxZ > blockMinZ && minZ < blockMaxZ) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    bool isBlockSolid(int x, int y, int z) {
        if (!world) return false;
        VoxelType blockType = world->getBlockTypeAt(x, y, z);
        return blockType != VoxelType::AIR;
    }
};

class DonutArray : public ModelArray<Donut> {
public:
    void init() {
        model = Donut(glm::vec3(0.0f), glm::vec3(0.05f, 0.025f, 0.025f));
        model.init();
    }

    void setWorld(World* world) {
        model.setWorld(world);
    }

    // Override render to include collision detection
    void render(Shader shader, float dt) {
        for (RigidBody& rb : instances) {
            // Set the rigid body for collision checking
            model.rb = rb;

            // Update physics with collision detection
            model.updatePhysics(dt);

            // Update the instance with the new position after collision
            rb = model.rb;

            // Render the model
            model.render(shader, dt);
        }
    }
};

#endif