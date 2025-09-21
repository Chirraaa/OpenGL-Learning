#include "RigidBody.h"



RigidBody::RigidBody(float mass, glm::vec3 pos, glm::vec3 velocity, glm::vec3 acceleration)
	: mass(mass), pos(pos), velocity(velocity), acceleration(acceleration) {
}

void RigidBody::update(float dt) {
	pos += velocity * dt + 0.5f * acceleration * (dt * dt);
	velocity += acceleration * dt;
}

void RigidBody::applyForce(glm::vec3 force) {
	acceleration += force / mass;
}

void RigidBody::applyForce(glm::vec3 direction, float magnitude) {
	applyForce(direction * magnitude);
}

void RigidBody::applyAcceleration(glm::vec3 acceleration) {
	this->acceleration += acceleration;
}

void RigidBody::applyAcceleration(glm::vec3 direction, float magnitude) {
	applyAcceleration(direction * magnitude);
}

void RigidBody::applyImpulse(glm::vec3 force, float dt) {
	velocity += force / mass * dt;
}

void RigidBody::applyImpulse(glm::vec3 direction, float magnitude, float dt) {
	applyImpulse(direction * magnitude, dt);
}

void RigidBody::transferEnergy(float joules) {
	if (joules == 0.0f) return;

	// comes from formula : KE = 0.5 * m * v^2
	float deltaV = sqrt(2.0f * abs(joules) / mass);

	velocity += (joules > 0.0f ? 1.0f : -1.0f) * glm::normalize(velocity) * deltaV;
}