#include "../Model.h"
#include "../../io/Camera.h"

class Gun : public Model {
public:
	Gun()
		: Model(glm::vec3(0.0f), glm::vec3(0.05f), true) {
	}

    void render(Shader shader, Camera* camera, float dt, bool setModel = false) {
        glm::vec3 front = camera->cameraFront;
        glm::vec3 up = camera->cameraUp;
        glm::vec3 right = camera->cameraRight;

        glm::mat4 cameraRotation = glm::mat4(
            glm::vec4(right, 0.0f),
            glm::vec4(up, 0.0f),
            glm::vec4(front, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );


        glm::mat4 model = glm::mat4(1.0f);

        model = glm::scale(model, size);

        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        model = cameraRotation * model;

        rb.pos = camera->cameraPos + (front * 0.2f) + (up * -0.1f) + (right * 0.3f);
        model = glm::translate(glm::mat4(1.0f), rb.pos) * model;

        shader.setMat4("model", model);
        Model::render(shader, dt, false);
    }
};