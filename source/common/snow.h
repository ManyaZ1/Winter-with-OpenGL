// common/snow.h
#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <GL/glew.h>

struct SnowParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float lifetime;
};

class SnowSystem {
private:
    std::vector<SnowParticle> particles;
    GLuint VAO, VBO;
    GLuint shaderProgram;
    GLuint snowTexture;
    int maxParticles;
    bool isSnowing;

    // Spawn area bounds
    glm::vec3 spawnMin;
    glm::vec3 spawnMax;
    float spawnHeight;

public:
    SnowSystem(int maxParticles = 5000);
    ~SnowSystem();

    void initialize(GLuint program); //, GLuint texture
    void update(float deltaTime, glm::vec3 cameraPos);
    void update_velocity(float x_offset, float z_offset);
    void render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    void toggle(); // Start/stop snowing
    bool isActive() { return isSnowing; }

private:
    void resetParticle(SnowParticle& p, glm::vec3 cameraPos);
    void initBuffers();
};
