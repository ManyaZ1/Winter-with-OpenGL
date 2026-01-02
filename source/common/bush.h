#ifndef BUSH_H
#define BUSH_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "model.h"

struct BushInstance {
    glm::vec3 position;
    float rotationY;
    float scale;
    int textureIndex;
};

class BushField {
public:
    BushField(Drawable* model, GLuint shaderID, int count);
    ~BushField();

    void setTerrainBounds(float minX_, float maxX_,
        float minZ_, float maxZ_,
        float minY_, float maxY_,
        float scale);

    void loadHeightData(const std::vector<float>& heights, int resolution);
    bool loadTerrainBinary(const std::string& filePath);
    void setTreeReferences(const std::vector<glm::vec3>& treePositions);
    bool isFarFromExistingBush(float x, float z) const;
    void generate();
    void draw();

private:
    bool isValidPlacement(float x, float z, float y) const;
    bool isFarFromTrees(float x, float z) const;
    void setupInstancing();
    void uploadToGPU();
    float sampleHeight(float x, float z) const;
    float sampleMask(const std::vector<float>& mask, float x, float z) const;

private:
    Drawable* bushModel;
    GLuint shader;
    int targetInstanceCount;
    std::vector<BushInstance> instances;
    std::vector<glm::mat4> modelMatrices;
    std::vector<int> textureIndices;

    std::vector<float> heightData;
    std::vector<float> forestMask;
    std::vector<float> lakeMask;

    std::vector<glm::vec3> treePositions;

    // GPU buffers
    GLuint instanceVBO;
    GLuint textureVBO;

    // Terrain data

    int gridResolution;
    float scalingFactor;

    // Terrain bounds
    float minX, maxX, minZ, maxZ, minY, maxY;
};

#endif //BUSH_H