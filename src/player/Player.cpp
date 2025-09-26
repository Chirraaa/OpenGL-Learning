#include "Player.h"
#include "../graphics/env/World.h"

Player::Player(glm::vec3 startPos, Camera* cam, World* gameWorld) :
    position(startPos),
    camera(cam),
    world(gameWorld),
    width(0.8f), height(1.8f), depth(0.8f),
    velocity(0.0f),
    onGround(false) {

    //// Find a safe spawn position above the terrain
    float terrainHeight = world->getTerrainHeight(startPos.x, startPos.z);
    position.y = terrainHeight + height + 10.0f; // Spawn 2 blocks above terrain


    updateCamera();
}

Camera* Player::getCamera() {
    return camera;
}

void Player::update(float dt) {
    glm::vec3 prevPosition = position;

    // Apply gravity
    velocity.y += GRAVITY * dt;

    // Terminal velocity
    if (velocity.y < TERMINAL_VELOCITY) {
        velocity.y = TERMINAL_VELOCITY;
    }

    // Apply friction
    if (onGround) {
        velocity.x *= GROUND_FRICTION;
        velocity.z *= GROUND_FRICTION;
    }
    else {
        velocity.x *= AIR_FRICTION;
        velocity.z *= AIR_FRICTION;
    }

    // Stop tiny movements
    if (std::abs(velocity.x) < 0.01f) velocity.x = 0.0f;
    if (std::abs(velocity.z) < 0.01f) velocity.z = 0.0f;

    // Calculate new position
    glm::vec3 newPosition = position + velocity * dt;

    // Resolve collisions
    resolveCollisions(newPosition);

    position = newPosition;
    validatePosition();
    updateCamera();
}

void Player::move(glm::vec3 direction) {
    if (glm::length(direction) > 0.0f) {
        direction = glm::normalize(direction);

        // Apply movement in world space, not camera space
        velocity.x = direction.x * MOVE_SPEED;
        velocity.z = direction.z * MOVE_SPEED;
    }
}

void Player::validatePosition() {
    float terrainHeight = world->getTerrainHeight(position.x, position.z);
    // If player is below terrain, teleport to safe position
    if (position.y < terrainHeight) {
        teleportToSafePosition();
        std::cout << "Player position corrected - was below terrain" << std::endl;
    }
}

void Player::jump() {
    if (onGround && canJump) {
        velocity.y = JUMP_FORCE;
        onGround = false;
        canJump = false;
    }
}

void Player::resolveCollisions(glm::vec3& newPosition) {
    glm::vec3 testPos = position;

    // Test X movement
    testPos.x = newPosition.x;
    if (checkCollision(testPos)) {
        newPosition.x = position.x;
        velocity.x = 0.0f;
    }

    // Test Y movement (gravity/jumping)
    testPos = glm::vec3(newPosition.x, newPosition.y, position.z);
    if (checkCollision(testPos)) {
        if (velocity.y < 0.0f) {
            // Falling - land on top of block
            newPosition.y = std::floor(position.y);
            onGround = true;
            canJump = true;
        }
        else if (velocity.y > 0.0f) {
            // Rising - hit ceiling
            newPosition.y = std::floor(newPosition.y + height) - height;
        }
        velocity.y = 0.0f;
    }
    else {
        onGround = false;
    }

    // Test Z movement
    testPos = glm::vec3(newPosition.x, newPosition.y, newPosition.z);
    if (checkCollision(testPos)) {
        newPosition.z = position.z;
        velocity.z = 0.0f;
    }
}

bool Player::checkCollision(glm::vec3 newPosition) {
    float minX = newPosition.x - width / 2.0f;
    float maxX = newPosition.x + width / 2.0f;
    float minY = newPosition.y;
    float maxY = newPosition.y + height;
    float minZ = newPosition.z - depth / 2.0f;
    float maxZ = newPosition.z + depth / 2.0f;

    for (int x = static_cast<int>(floor(minX)); x <= static_cast<int>(floor(maxX)); x++) {
        for (int y = static_cast<int>(floor(minY)); y <= static_cast<int>(floor(maxY)); y++) {
            for (int z = static_cast<int>(floor(minZ)); z <= static_cast<int>(floor(maxZ)); z++) {
                if (isBlockSolid(x, y, z)) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Player::isBlockSolid(int x, int y, int z) {
    // Check if we're below terrain level
    float terrainHeight = world->getTerrainHeight(x, z);
    if (y > terrainHeight) {
        return false; // Air above terrain
    }

    VoxelType blockType = world->getBlockType(x, y, z, terrainHeight);
    return blockType != VoxelType::AIR;
}

void Player::updateCamera() {
    // Position camera at player's eye level (near the top of the player)
    glm::vec3 cameraPos = position + glm::vec3(0.0f, height - 0.2f, 0.0f);
    camera->cameraPos = cameraPos;
}

void Player::teleportToSafePosition() {
    // Find terrain height at current X, Z position
    float terrainHeight = world->getTerrainHeight(position.x, position.z);

    // Place player safely above terrain
    position.y = terrainHeight + height + 1.0f;
    velocity.y = 0.0f; // Stop any falling

    updateCamera();
}

glm::vec3 Player::getPosition() const { return position; }
glm::vec3 Player::getVelocity() const { return velocity; }
glm::vec3 Player::getAcceleration() const { return acceleration; }
bool Player::isOnGround() const { return onGround; }