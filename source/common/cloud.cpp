#include "cloud.h"
#include <common/texture.h>
#include <iostream>
//#include <common/model.h>
CloudSystem::CloudSystem() : cloudQuad(nullptr), cloudTexture(0) {}

CloudSystem::~CloudSystem() {
    if (cloudQuad) delete cloudQuad;
    if (cloudTexture) glDeleteTextures(1, &cloudTexture);
}

void CloudSystem::initialize(GLuint shaderProgram) {
    // Create quad geometry
    vector<vec3> quadVerts = {
        vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0),
        vec3(1, 1, 0), vec3(-1, 1, 0), vec3(-1, -1, 0)
    };

    vector<vec2> quadUVs = {
        vec2(0, 0), vec2(1, 0), vec2(1, 1),
        vec2(1, 1), vec2(0, 1), vec2(0, 0)
    };

    cloudQuad = new Drawable(quadVerts, quadUVs);

    // Load cloud texture
    cloudTexture = loadSOIL("assets/cloud2.png");
    glBindTexture(GL_TEXTURE_2D, cloudTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Get shader uniform locations
    modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
    useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");
    textureLocation = glGetUniformLocation(shaderProgram, "sunTex");

    cout << "CloudSystem initialized" << endl;
}

void CloudSystem::addCloud(vec3 position, float size) {
    Cloud cloud;
    cloud.position = position;
    cloud.size = size;
    cloud.alpha = 0.5f;
    cloud.speed = 0.5f + (rand() % 10) / 10.0f; // Random speed 0.5-1.5
    clouds.push_back(cloud);
}

void CloudSystem::update(float deltaTime) {
    for (auto& cloud : clouds) {
        // Move cloud
        cloud.position.x += cloud.speed * deltaTime;

        // Wrap around
        if (cloud.position.x > 50.0f) {
            cloud.position.x = -50.0f;
        }
    }
}

void CloudSystem::render(mat4 viewMatrix, mat4 projectionMatrix) {
    if (clouds.empty()) return;

    // Enable blending for transparency
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Set texture mode and bind cloud texture
    glUniform1i(useTextureLocation, 4);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, cloudTexture);
    glUniform1i(textureLocation, 6);

    // Extract camera orientation for billboarding
    vec3 cameraRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    vec3 cameraUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    vec3 cameraForward = cross(cameraRight, cameraUp);

    // Render each cloud
    for (const auto& cloud : clouds) {
        // Build billboard matrix
        mat4 billboard = mat4(1.0f);
        billboard[0] = vec4(cameraRight * cloud.size, 0);
        billboard[1] = vec4(cameraUp * cloud.size, 0);
        billboard[2] = vec4(cameraForward * cloud.size, 0);
        billboard[3] = vec4(cloud.position, 1);

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &billboard[0][0]);

        cloudQuad->bind();
        cloudQuad->draw();
    }

    // Restore OpenGL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
