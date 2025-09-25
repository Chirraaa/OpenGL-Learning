#include "Player.h"
#include "../physics/Environment.h"
#include <algorithm>
#include <iostream>

Player::Player(glm::vec3 startPos, float playerHeight, float playerWidth)
    : rb(70.0f, startPos), // 70kg default mass
    height(playerHeight),
    width(playerWidth),
    depth(playerWidth),
    walkSpeed(5.0f),
    runSpeed(8.0f),
    jumpForce(300.0f),
    mouseSensitivity(0.1f),
    groundCheckDistance(0.1f),
    state(PlayerState::AIRBORNE),
    isRunning(false),
    canJump(false),
    groundNormal(glm::vec3(0.0f, 1.0f, 0.0f))
{
    // Create camera at eye level
    cameraOffset = glm::vec3(0.0f, height * 0.85f, 0.0f); // Eye level at 85% of height
    camera = new Camera(startPos + cameraOffset);

    // Apply gravity from the start
    rb.applyAcceleration(Environment::gravitationalAcceleration);

    std::cout << "Player created at position: ("
        << startPos.x << ", " << startPos.y << ", " << startPos.z << ")" << std::endl;
}

Player::~Player() {
    delete camera;
}

void Player::update(float deltaTime) {
    // Store previous position for collision resolution
    glm::vec3 previousPos = rb.pos;

    // Update physics
    rb.update(deltaTime);

    // Basic ground collision check (you'll replace this with proper collision system later)
    checkGroundCollision();

    // Apply friction when on ground
    if (state == PlayerState::GROUNDED) {
        applyFriction(deltaTime);
    }

    // Update player state
    updatePlayerState();

    // Update camera position to follow player
    updateCameraPosition();
}

void Player::processMovementInput(bool forward, bool backward, bool left, bool right, bool run, float deltaTime) {
    isRunning = run;

    // Calculate movement direction based on camera orientation
    glm::vec3 movement = calculateMovementDirection(forward, backward, left, right);

    if (glm::length(movement) > 0.0f) {
        // Normalize movement vector
        movement = glm::normalize(movement);

        // Apply speed
        float currentSpeed = isRunning ? runSpeed : walkSpeed;

        // Only apply horizontal movement (don't affect Y velocity directly)
        glm::vec3 horizontalForce = glm::vec3(movement.x, 0.0f, movement.z) * currentSpeed * rb.mass;

        // Apply movement force
        if (state == PlayerState::GROUNDED) {
            // On ground: apply full force
            rb.velocity.x = movement.x * currentSpeed;
            rb.velocity.z = movement.z * currentSpeed;
        }
        else {
            // In air: reduced control
            float airControl = 0.3f;
            rb.applyForce(horizontalForce * airControl * deltaTime);
        }
    }
}

void Player::processMouseInput(double deltaX, double deltaY) {
    camera->updateCameraDirection(deltaX * mouseSensitivity, deltaY * mouseSensitivity);
}

void Player::processJumpInput() {
    if (canJump && state == PlayerState::GROUNDED) {
        rb.applyImpulse(glm::vec3(0.0f, 1.0f, 0.0f), jumpForce, 1.0f);
        state = PlayerState::JUMPING;
        canJump = false;

        std::cout << "Player jumped!" << std::endl;
    }
}

void Player::processScrollInput(double deltaY) {
    camera->updateCameraZoom(deltaY);
}

void Player::applyGravity(float deltaTime) {
    // Gravity is already applied in the constructor via rb.applyAcceleration
    // This method exists for potential custom gravity effects
}

void Player::applyFriction(float deltaTime) {
    if (state == PlayerState::GROUNDED) {
        float friction = 10.0f; // Friction coefficient

        // Apply friction to horizontal velocity
        glm::vec3 horizontalVel = glm::vec3(rb.velocity.x, 0.0f, rb.velocity.z);
        float speed = glm::length(horizontalVel);

        if (speed > 0.0f) {
            glm::vec3 frictionForce = -glm::normalize(horizontalVel) * friction * rb.mass;
            rb.applyForce(frictionForce * deltaTime);

            // Prevent friction from reversing direction
            if (glm::length(glm::vec3(rb.velocity.x, 0.0f, rb.velocity.z)) < 0.1f) {
                rb.velocity.x = 0.0f;
                rb.velocity.z = 0.0f;
            }
        }
    }
}

void Player::checkGroundCollision() {
    // Simple ground collision at Y = 0 (you'll replace this with proper collision system)
    float groundLevel = 0.0f;
    float playerBottom = rb.pos.y - height * 0.5f;

    if (playerBottom <= groundLevel && rb.velocity.y <= 0.0f) {
        // Player is on ground
        rb.pos.y = groundLevel + height * 0.5f;
        rb.velocity.y = 0.0f;

        if (state != PlayerState::GROUNDED) {
            state = PlayerState::GROUNDED;
            canJump = true;
            std::cout << "Player landed on ground" << std::endl;
        }
    }
    else if (playerBottom > groundLevel + groundCheckDistance) {
        // Player is airborne
        if (state == PlayerState::GROUNDED) {
            state = PlayerState::AIRBORNE;
            canJump = false;
        }
    }
}

glm::vec3 Player::getCameraPosition() const {
    return camera->cameraPos;
}

glm::mat4 Player::getViewMatrix() const {
    return camera->getViewMatrix();
}

void Player::setPosition(glm::vec3 position) {
    rb.pos = position;
    updateCameraPosition();
}

void Player::teleport(glm::vec3 position) {
    rb.pos = position;
    rb.velocity = glm::vec3(0.0f);
    rb.acceleration = Environment::gravitationalAcceleration; // Reset acceleration
    updateCameraPosition();
}

void Player::updateCameraPosition() {
    camera->cameraPos = rb.pos + cameraOffset;
}

void Player::updatePlayerState() {
    // Update state based on velocity and ground contact
    if (state == PlayerState::JUMPING && rb.velocity.y <= 0.0f) {
        state = PlayerState::AIRBORNE;
    }
}

glm::vec3 Player::calculateMovementDirection(bool forward, bool backward, bool left, bool right) {
    glm::vec3 movement(0.0f);

    // Get camera's forward and right vectors (without Y component for movement)
    glm::vec3 cameraForward = glm::normalize(glm::vec3(camera->cameraFront.x, 0.0f, camera->cameraFront.z));
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));

    if (forward) movement += cameraForward;
    if (backward) movement -= cameraForward;
    if (right) movement += cameraRight;
    if (left) movement -= cameraRight;

    return movement;
}