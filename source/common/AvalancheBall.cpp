#include "AvalancheBall.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
// Link to sampling function in winter.cpp
extern float sampleHeightAt(float x, float z, const std::vector<float>& heightData, int gridResolution, float minX, float maxX, float minZ, float maxZ);

AvalancheBall::AvalancheBall(Drawable* sphereMesh, GLuint shaderProgram, const std::vector<float>& terrainData)
    : mesh(sphereMesh), heightMap(terrainData), active(false), radius(0.5f),
    inLake(false), meltTimer(0.0f), meltRate(1.0f / MELT_DURATION),
    startRadius(0.0f) {

    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
    KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
    KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
    NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
}

void AvalancheBall::spawn(glm::vec3 pos) {
    position = pos;
    velocity = glm::vec3(0.0f);
    radius = 0.5f;
    active = true;
    inLake = false;
    meltTimer = 0.0f;
    startRadius = 0.0f;  // Reset on spawn
}
void AvalancheBall::update(float deltaTime) {
    if (!active) return;

    // 1. Calculate Slope (Downhill direction)
    float d = 0.5f;
    int res = 1024; float b = 100.0f; // Grid settings from winter.cpp

    float hL = sampleHeightAt(position.x - d, position.z, heightMap, res, -b, b, -b, b);
    float hR = sampleHeightAt(position.x + d, position.z, heightMap, res, -b, b, -b, b);
    float hBack = sampleHeightAt(position.x, position.z - d, heightMap, res, -b, b, -b, b);
    float hFront = sampleHeightAt(position.x, position.z + d, heightMap, res, -b, b, -b, b);

    // Vector pointing from high ground to low ground
    glm::vec3 slopeDir(hL - hR, 0.0f, hBack - hFront);

    // 2. Physics logic
    if (glm::length(slopeDir) > 0.01f) {
        velocity += glm::normalize(slopeDir) * GRAVITY * deltaTime;
    }

    position += velocity * deltaTime;
    velocity *= 0.99f; // Slight drag

    // 3. Growth & State Check
    float ground = sampleHeightAt(position.x, position.z, heightMap, res, -b, b, -b, b);
    float speed = glm::length(velocity);
    bool inWater = (ground <= (LAKE_LEVEL + 0.5f));
    bool isStuck = (speed < 0.2f); // Threshold for being "stuck" on land

    // Only grow if moving and not in water/melting
    if (!inWater && !isStuck && meltTimer == 0.0f && radius < MAX_RADIUS) {
        radius += speed * deltaTime * 0.07f;
    }

    // 4. Melting Logic (Triggered by Water OR being Stuck)
    if (inWater || isStuck || meltTimer > 0.0f) {
        // Initialize melting state
        if (meltTimer == 0.0f) {
            startRadius = radius;
        }

        if (inWater) {
            velocity *= 0.9f; // Water resistance
            inLake = true;
        }

        // Melt faster in water than on land
        float meltSpeedMultiplier = inWater ? 1.0f : 0.5f;
        meltTimer += deltaTime * meltSpeedMultiplier;

        float meltFactor = glm::clamp(meltTimer / MELT_DURATION, 0.0f, 1.0f);

        // Shrink using the captured startRadius
        radius = startRadius * (1.0f - meltFactor);

        // Sink slightly if in water, otherwise stay on ground
        if (inWater) {
            position.y = ground + (radius * 0.5f) - (meltFactor * 0.5f);
        }
        else {
            position.y = ground + (radius * 0.8f);
        }

        // Deactivate when finished
        if (meltFactor >= 1.0f || radius < 0.05f) {
            active = false;
            return;
        }
    }
    else {
        // Normal state: No melting, regular terrain clamping
        position.y = ground + (radius * 0.8f);
    }
}
//void AvalancheBall::update(float deltaTime) {
//    if (!active) return;
//
//    // 1. Calculate Slope (Downhill direction)
//    float d = 0.5f;
//    int res = 1024; float b = 100.0f; // Grid settings from winter.cpp
//
//    float hL = sampleHeightAt(position.x - d, position.z, heightMap, res, -b, b, -b, b);
//    float hR = sampleHeightAt(position.x + d, position.z, heightMap, res, -b, b, -b, b);
//    float hBack = sampleHeightAt(position.x, position.z - d, heightMap, res, -b, b, -b, b);
//    float hFront = sampleHeightAt(position.x, position.z + d, heightMap, res, -b, b, -b, b);
//
//    // Vector pointing from high ground to low ground
//    glm::vec3 slopeDir(hL - hR, 0.0f, hBack - hFront);
//
//    // 2. Physics logic
//    if (glm::length(slopeDir) > 0.01f) {
//        velocity += glm::normalize(slopeDir) * GRAVITY * deltaTime;
//    }
//
//    position += velocity * deltaTime;
//    velocity *= 0.99f; // Slight drag
//
//    // 3. Growth
//    if (!inLake && radius < MAX_RADIUS) {
//        radius += glm::length(velocity) * deltaTime * 0.1f;
//    }
//
//    // 4. Terrain Clamping
//    float ground = sampleHeightAt(position.x, position.z, heightMap, res, -b, b, -b, b);
//    position.y = ground + (radius * 0.8f); // Sits slightly in the snow
//    // 5. Lake Check & Melting Logic
//    if (ground <= (LAKE_LEVEL + 0.5f)) { // Added a small epsilon
//        if (!inLake) {
//            inLake = true;
//            startRadius = radius; // Capture the radius the moment it hits water
//        }
//
//        velocity *= 0.95f; // Stronger water resistance
//
//        // Increment melt timer regardless of speed, 
//        // but maybe speed it up as it slows down
//        meltTimer += deltaTime;
//
//        float meltFactor = glm::clamp(meltTimer / MELT_DURATION, 0.0f, 1.0f);
//
//        // Shrink using the instance-specific startRadius
//        radius = startRadius * (1.0f - meltFactor);
//
//        // Sink the ball: as it melts, it should drop lower into the water
//        position.y = ground + (radius * 0.5f) - (meltFactor * 0.5f);
//
//        if (meltFactor >= 1.0f || radius < 0.05f) {
//            active = false;
//            return;
//        }
//    }
//    else {
//        inLake = false;
//        meltTimer = 0.0f; // Reset if it somehow rolls out of the lake
//        position.y = ground + (radius * 0.8f);
//    }
//
//}
//glm::mat4 AvalancheBall::getModelMatrix() const {
//    return glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
//}
glm::mat4 AvalancheBall::getModelMatrix() const {
    float visualRadius = radius;

    // Apply squashing if the ball has started melting
    if (meltTimer > 0.0f) {
        float meltFactor = glm::clamp(meltTimer / MELT_DURATION, 0.0f, 1.0f);

        // 1. Squash vertically (Y-axis)
        // We multiply by a factor that goes from 1.0 down to, say, 0.3
        float yScale = glm::mix(1.0f, 0.3f, meltFactor);

        // 2. Expand horizontally (X and Z axes)
        // We multiply by a factor that goes from 1.0 up to 1.3 to simulate spreading
        float xzScale = glm::mix(1.0f, 1.3f, meltFactor);

        return glm::translate(glm::mat4(1.0f), position) *
            glm::scale(glm::mat4(1.0f), glm::vec3(visualRadius * xzScale, visualRadius * yScale, visualRadius * xzScale));
    }

    // Normal sphere scaling when not melting
    return glm::translate(glm::mat4(1.0f), position) *
        glm::scale(glm::mat4(1.0f), glm::vec3(visualRadius));
}
void AvalancheBall::draw() {
    if (!active) return;
    glUniform1i(useTextureLocation, 8);
    //glm::mat4 model = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), glm::vec3(radius));
    glm::mat4 model = getModelMatrix();
    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &model[0][0]);

    // Snowy Material
    glUniform4f(KaLocation, 0.5f, 0.5f, 0.5f, 1.0f);
    glUniform4f(KdLocation, 0.9f, 0.9f, 0.9f, 1.0f);
    glUniform4f(KsLocation, 0.2f, 0.2f, 0.2f, 1.0f);
    glUniform1f(NsLocation, 10.0f);

    mesh->bind(); mesh->draw();
}