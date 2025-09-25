#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>
#include "../physics/RigidBody.h"
#include "../io/Camera.h"

enum class PlayerState {
    GROUNDED,
    AIRBORNE,
    JUMPING
};

class Player {
public:
    // Physics body for the player
    RigidBody rb;

    // Camera attached to the player
    Camera* camera;

    // Player properties
    float walkSpeed;
    float runSpeed;
    float jumpForce;
    float mouseSensitivity;

    // Player dimensions for collision
    float height;
    float width;
    float depth;

    // State tracking
    PlayerState state;
    bool isRunning;
    bool canJump;

    // Ground detection
    float groundCheckDistance;
    glm::vec3 groundNormal;

    // Constructor
    Player(glm::vec3 startPos = glm::vec3(0.0f, 0.0f, 0.0f),
        float playerHeight = 1.8f,
        float playerWidth = 0.6f);

    // Destructor
    ~Player();

    // Update player physics and camera
    void update(float deltaTime);

    // Input handling
    void processMovementInput(bool forward, bool backward, bool left, bool right, bool run, float deltaTime);
    void processMouseInput(double deltaX, double deltaY);
    void processJumpInput();
    void processScrollInput(double deltaY);

    // Physics methods
    void applyGravity(float deltaTime);
    void applyFriction(float deltaTime);
    void checkGroundCollision();

    // Getters
    glm::vec3 getPosition() const { return rb.pos; }
    glm::vec3 getVelocity() const { return rb.velocity; }
    glm::vec3 getCameraPosition() const;
    glm::mat4 getViewMatrix() const;

    // Setters
    void setPosition(glm::vec3 position);
    void teleport(glm::vec3 position);

private:
    // Camera offset from player position (eye level)
    glm::vec3 cameraOffset;

    // Internal methods
    void updateCameraPosition();
    void updatePlayerState();
    glm::vec3 calculateMovementDirection(bool forward, bool backward, bool left, bool right);
};

#endif