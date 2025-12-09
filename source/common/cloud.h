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
};

class CloudSystem {
private:
    vector<Cloud> clouds;
    Drawable* cloudQuad;
    GLuint cloudTexture;

    // Shader uniform locations
    GLuint modelMatrixLocation;
    GLuint useTextureLocation;
    GLuint textureLocation;

public:
    CloudSystem();
    ~CloudSystem();

    void initialize(GLuint shaderProgram);
    void addCloud(vec3 position, float size = 5.0f);
    void update(float deltaTime);
    void render(mat4 viewMatrix, mat4 projectionMatrix);

    int getCloudCount() const { return clouds.size(); }
};

#endif
