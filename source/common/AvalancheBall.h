#ifndef AVALANCHE_BALL_H
#define AVALANCHE_BALL_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <common/model.h> 
#include <vector>

class AvalancheBall {
public:
    // Pass the shared height data so the ball knows where the ground is
    AvalancheBall(Drawable* sphereMesh, GLuint shaderProgram, const std::vector<float>& terrainData);

    void spawn(glm::vec3 pos);
    void update(float deltaTime);
    void draw();
    glm::mat4 getModelMatrix() const;
    bool isActive() const { return active; }

private:
    Drawable* mesh;
    const std::vector<float>& heightMap; // Reference to winter.cpp's heightData

    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
    bool active;

	//melt the avalanche ball when it hits the lake
    bool inLake;
    float meltTimer;
    float meltRate;
    float startRadius;
    // Uniform Locations
    GLuint modelMatrixLocation;
    GLuint KaLocation, KdLocation, KsLocation, NsLocation;
    GLuint useTextureLocation;

    // Constants
    const float GRAVITY = 18.0f;
    const float MAX_RADIUS = 7.0f;
    const float LAKE_LEVEL = 3.21f; // Matching your winter.cpp waterHeight
    const float MELT_DURATION = 15.0f; // Time to fully melt (seconds)
};

#endif