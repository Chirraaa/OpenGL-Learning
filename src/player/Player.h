#include <glm/glm.hpp>
#include "../io/Camera.h"
#include "../physics/RigidBody.h"
#include "../graphics/env/World.h"

class Player {

private:
	Camera* camera;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;

	// Physics constants
	const float GRAVITY = -32.0f;
	const float AIR_FRICTION = 0.98f;
	const float GROUND_FRICTION = 0.95f;
	const float TERMINAL_VELOCITY = -50.0f;
	const float MOVE_SPEED = 5.0f;
	const float JUMP_FORCE = 10.0f;

	// Player dimensions (bounding box)
	float width;
	float height;
	float depth;

	bool onGround = false;
	bool canJump = false;

	World* world;
public:
	Player(glm::vec3 startPos, Camera* cam, World* gameWorld);

	// update physics and movement
	void update(float dt);

	// movement input
	void move(glm::vec3 direction);

	void jump();

	// collision detection
	bool checkCollision(glm::vec3 newPosition);
	bool isBlockSolid(int x, int y, int z);
	void resolveCollisions(glm::vec3& newPosition);

	// Teleport player to safe position if stuck
	void teleportToSafePosition();

	void validatePosition();
	// getters
	Camera* getCamera();
	glm::vec3 getPosition() const;
	glm::vec3 getVelocity() const;
	glm::vec3 getAcceleration() const;
	bool isOnGround() const;

private:
	void updateCamera();
};