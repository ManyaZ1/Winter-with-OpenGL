#ifndef CLOUD_H
#define CLOUD_H

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <common/model.h>

using namespace glm;
using namespace std;

struct Cloud {
    vec3 position;
    float size;
    float alpha;
    float speed;
	float rotationAngle; //unused for now but can be used to rotate UVs in shader for more variety
    int textureID; // 0 for first image, 1 for second
};

class CloudSystem {
private:
    vector<Cloud> clouds;
    Drawable* cloudQuad;
    //GLuint cloudTexture;
    GLuint cloudTextures[2];
    // Shader uniform locations
    GLuint modelMatrixLocation;
    GLuint useTextureLocation;
    GLuint textureLocation;
    GLuint uvRotationLocation;

public:
    CloudSystem();
    ~CloudSystem();

    void initialize(GLuint shaderProgram);
    void addCloud(vec3 position, float size = 5.0f);
    void update(float deltaTime);
    void render(mat4 viewMatrix, mat4 projectionMatrix);
    void clearClouds();
    int getCloudCount() const { return clouds.size(); }
    
};

#endif
