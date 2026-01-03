// common/snow.cpp
#include "snow.h"
#include <common/texture.h>
#include <random>
#include <iostream>
using namespace glm;

SnowSystem::SnowSystem(int max) : maxParticles(max), isSnowing(false) {
    particles.resize(maxParticles);
    spawnHeight = 40.0f; // Height where snow spawns (lowered from 100)
}

SnowSystem::~SnowSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void SnowSystem::initialize(GLuint program, GLuint texture) {
    shaderProgram = program;

    // Load snow texture (simple white circle/snowflake)
    snowTexture = texture; //loadSOIL("assets/snowflake.png");

    initBuffers();
}

void SnowSystem::initBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Allocate space for particle data
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(SnowParticle),
        nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}

void SnowSystem::resetParticle(SnowParticle& p, glm::vec3 cameraPos) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> distX(-50.0f, 50.0f);
    static std::uniform_real_distribution<float> distZ(-50.0f, 50.0f);
    static std::uniform_real_distribution<float> distSize(0.1f, 0.3f);
    static std::uniform_real_distribution<float> distVel(2.0f, 5.0f);

    // Spawn around camera position
    p.position = glm::vec3(
        cameraPos.x + distX(gen),
        spawnHeight,
        cameraPos.z + distZ(gen)
    );

    // Falling velocity with slight horizontal drift
    p.velocity = glm::vec3(
        (distX(gen) * 0.02f),  // Slight X drift
        -distVel(gen),         // Falling speed
        (distZ(gen) * 0.02f)   // Slight Z drift
    );

    p.size = distSize(gen);
    p.lifetime = 0.0f;
}

void SnowSystem::update(float deltaTime, glm::vec3 cameraPos) {
    if (!isSnowing) return;

    for (auto& p : particles) {
        // Update position
        p.position += p.velocity * deltaTime;
        p.lifetime += deltaTime;

        // Reset if particle goes too low or too far from camera
        if (p.position.y < 0.0f ||
            glm::distance(glm::vec2(p.position.x, p.position.z),
                glm::vec2(cameraPos.x, cameraPos.z)) > 100.0f) {
            resetParticle(p, cameraPos);
        }
    }
}

void SnowSystem::render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
    if (!isSnowing) {
        std::cout << "Snow render skipped - not snowing" << std::endl;
        return;
    }
    std::cout << "Rendering " << particles.size() << " snow particles" << std::endl;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer

    glUseProgram(shaderProgram);

    // Get uniform locations for snow shader
    GLuint vpLocation = glGetUniformLocation(shaderProgram, "VP");
    GLuint texLocation = glGetUniformLocation(shaderProgram, "snowTexture");
    GLuint pointSizeLocation = glGetUniformLocation(shaderProgram, "pointSize");

    // Upload uniforms
    glm::mat4 VP = projectionMatrix * viewMatrix;
    glUniformMatrix4fv(vpLocation, 1, GL_FALSE, &VP[0][0]);
    //Set M to Identity because p.position is already in World Space
    glm::mat4 IdentityM = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "M"), 1, GL_FALSE, &IdentityM[0][0]);
    // Bind snow texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snowTexture);
    glUniform1i(texLocation, 0);

    // Set point size
    glUniform1f(pointSizeLocation, 180.0f); // Adjust for visibility

    // Enable point sprites
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);

    // Prepare vertex data (just positions)
    std::vector<glm::vec3> positions;
    positions.reserve(particles.size());
    for (const auto& p : particles) {
        positions.push_back(p.position);
    }

    // Upload to GPU
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3),
        positions.data(), GL_DYNAMIC_DRAW);

    // Set vertex attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // Draw all particles as points
    glDrawArrays(GL_POINTS, 0, particles.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);//?
    glBindVertexArray(0);

    glDisable(GL_POINT_SPRITE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SnowSystem::toggle() {
    isSnowing = !isSnowing;
    std::cout << "SnowSystem::toggle() - isSnowing = " << isSnowing << std::endl;
    if (isSnowing) {
        // Initialize all particles when starting
        glm::vec3 initPos(0, spawnHeight, 0);
        for (auto& p : particles) {
            resetParticle(p, initPos);
        }
        std::cout << "Initialized " << particles.size() << " particles" << std::endl;
    }
}