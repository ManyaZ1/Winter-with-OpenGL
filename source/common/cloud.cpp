#include "cloud.h"
#include <common/texture.h>
#include <iostream>
//#include <common/model.h>
//CloudSystem::CloudSystem() : cloudQuad(nullptr), cloudTexture(0) {}

CloudSystem::CloudSystem() : cloudQuad(nullptr) {
    cloudTextures[0] = 0;
    cloudTextures[1] = 0;
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
    //cloudTexture = loadSOIL("assets/cloud2.png"); //cloud more fluffy cloud2 more angry looking  
    cloudTextures[0] = loadSOIL("assets/cloud.png"); 
    cloudTextures[1] = loadSOIL("assets/cloud2.png");
    // Set parameters for both (repeat for cloudTextures[1])
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, cloudTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    //glBindTexture(GL_TEXTURE_2D, cloudTexture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    uvRotationLocation = glGetUniformLocation(shaderProgram, "uvRotationAngle");
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
    // limit between 210 and 230
	//float randomDegree = (float)(rand() % 20) - 10.0f; //+-10 degrees
    //cloud.rotationAngle = (float)(220 +randomDegree);
    cloud.rotationAngle = 3.14159f ;
    //cloud.rotationAngle = (float)(rand() % 360) * 3.14159f / 180.0f;
	cloud.textureID = (rand() % 3) % 2; //66% is 1 or 2 and 33% it is 0. so 1%2=1 2%2=0 and 0%2=0 //Randomly choose 0 or 1 but 0 with 66% chance

    // or cloud.textureID = (rand() % 100 < 70) ? 0 : 1;
    clouds.push_back(cloud);
}

void CloudSystem::update(float deltaTime) {
    for (auto& cloud : clouds) {
        // Move cloud
        cloud.position.x += cloud.speed * deltaTime;

        // Wrap around in both directions
        if (cloud.position.x > 100.0f) {
            cloud.position.x = -100.0f;
        }
        else if (cloud.position.x < -100.0f) {
            cloud.position.x = 100.0f;
        }
    }
}
void CloudSystem::render(mat4 viewMatrix, mat4 projectionMatrix) {
    if (clouds.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glUniform1i(useTextureLocation, 4);

    vec3 cameraRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    vec3 cameraUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    vec3 cameraForward = cross(cameraRight, cameraUp);

    for (const auto& cloud : clouds) {
        mat4 billboard = mat4(1.0f);
        billboard[0] = vec4(cameraRight * cloud.size, 0); // camera right vector * size of cloud
        billboard[1] = vec4(cameraUp * cloud.size, 0);
        billboard[2] = vec4(cameraForward * cloud.size, 0); // Ζ (βάθος).
        billboard[3] = vec4(cloud.position, 1);

        // Bind the specific texture assigned to this cloud
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, cloudTextures[cloud.textureID]);
        glUniform1i(textureLocation, 6);

        // Set uniforms and draw
        glUniform1f(uvRotationLocation, cloud.rotationAngle);
        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &billboard[0][0]);

        cloudQuad->bind();
        cloudQuad->draw();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}


CloudSystem::~CloudSystem() {
    if (cloudQuad) {
        delete cloudQuad;
    }
    // Delete the array of 2 textures
    glDeleteTextures(2, cloudTextures);
}
// drive clouds away 
// clpouds spped up and alpha decreases WHEN they reach the x=100 boundary or z =100 boundary they disappear
void CloudSystem::clearClouds() {
    CloudSystem::clouds.clear();
}
