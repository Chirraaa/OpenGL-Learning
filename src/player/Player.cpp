#include "Player.h"
#include "../graphics/env/World.h"

Player::Player(glm::vec3 startPos, Camera* cam, World* gameWorld) :
	position(startPos),
	camera(cam),
	world(gameWorld),
	width(0.8f), height(1.8f), depth(0.8f),
	FOV(1.0f),
	velocity(0.0f),
	onGround(false),
	gravityEnabled(true) {

	// Ensure player starts at a valid position
	// Find terrain height at spawn position
	//float terrainHeight = world->getTerrainHeight(startPos.x, startPos.z);

	// Place player on top of terrain
	//position.y = terrainHeight + 1.0f;

	std::cout << "Player spawned at: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
	//std::cout << "Terrain height at spawn: " << terrainHeight << std::endl;
	camera->updateCameraZoom(FOV);
	updateCamera();
}

Camera* Player::getCamera() {
	return camera;
}

void Player::update(float dt) {
	glm::vec3 prevPosition = position;

	if (!gravityEnabled) {
		// Flying/No Gravity Mode - simple movement with no physics

		// Apply air friction to all axes in fly mode
		velocity.x *= AIR_FRICTION;
		velocity.y *= AIR_FRICTION;
		velocity.z *= AIR_FRICTION;

		// Stop tiny movements
		if (std::abs(velocity.x) < 0.01f) velocity.x = 0.0f;
		if (std::abs(velocity.y) < 0.01f) velocity.y = 0.0f;
		if (std::abs(velocity.z) < 0.01f) velocity.z = 0.0f;

		// Calculate new position (no collision detection in fly mode)
		position += velocity * dt;

		// Set onGround to false when flying
		onGround = false;
	}
	else {
		// Normal Physics Mode - with gravity and collisions

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
	}

	updateCamera();
}

void Player::move(glm::vec3 direction) {
	if (glm::length(direction) > 0.0f) {
		direction = glm::normalize(direction);

		if (!gravityEnabled) {
			// Flying mode - direct velocity control
			velocity.x = direction.x * FLY_SPEED;
			velocity.z = direction.z * FLY_SPEED;
		}
		else {
			// Normal mode - ground-based movement
			velocity.x = direction.x * MOVE_SPEED;
			velocity.z = direction.z * MOVE_SPEED;
		}
	}
}

void Player::moveVertical(float direction) {
	// Only allow vertical movement when gravity is disabled
	if (!gravityEnabled) {
		velocity.y = direction * FLY_SPEED;
	}
}

void Player::jump() {
	if (!gravityEnabled) {
		// In fly mode, jump acts as "fly up"
		velocity.y = FLY_SPEED;
	}
	else {
		// Normal jumping - only when on ground
		if (onGround && canJump) {
			velocity.y = JUMP_FORCE;
			onGround = false;
			canJump = false;
		}
	}
}

void Player::resolveCollisions(glm::vec3& newPosition) {
	if (!gravityEnabled) {
		return;
	}

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
	// Player bounding box
	float minX = newPosition.x - width / 2.0f;
	float maxX = newPosition.x + width / 2.0f;
	float minY = newPosition.y;
	float maxY = newPosition.y + height;
	float minZ = newPosition.z - depth / 2.0f;
	float maxZ = newPosition.z + depth / 2.0f;

	// Check all blocks that might intersect with player
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

					// Check if player bounding box intersects with block bounding box
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

bool Player::isBlockSolid(int x, int y, int z) {
	VoxelType blockType = world->getBlockTypeAt(x, y, z);
	return blockType != VoxelType::AIR;
}

void Player::updateCamera() {
	glm::vec3 cameraPos = position + glm::vec3(0.0f, height - 0.2f, 0.0f);
	camera->cameraPos = cameraPos;
}

void Player::teleportToSafePosition() {
	float terrainHeight = world->getTerrainHeight(position.x, position.z);

	position.y = terrainHeight + height + 1.0f;
	velocity.y = 0.0f;

	updateCamera();
}

void Player::validatePosition() {
	float terrainHeight = world->getTerrainHeight(position.x, position.z);
	if (position.y < terrainHeight) {
		teleportToSafePosition();
		std::cout << "Player position corrected - was below terrain" << std::endl;
	}
}

void Player::setGravityEnabled(bool enabled) {
	gravityEnabled = enabled;
	std::cout << "Gravity " << (enabled ? "ENABLED" : "DISABLED") << std::endl;

	if (enabled) {
		if (velocity.y > 0) {
			velocity.y = 0.0f;
		}
	}
	else {
		if (velocity.y < 0) {
			velocity.y = 0.0f;
		}
		onGround = false;
	}
}

bool Player::isGravityEnabled() const {
	return gravityEnabled;
}

// Getters
glm::vec3 Player::getPosition() const { return position; }
glm::vec3 Player::getVelocity() const { return velocity; }
glm::vec3 Player::getAcceleration() const { return acceleration; }
bool Player::isOnGround() const { return onGround; }