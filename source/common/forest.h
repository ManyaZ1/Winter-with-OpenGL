#ifndef FOREST_H
#define FOREST_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include "model.h"

struct TreeInstance {
    glm::vec3 position;
    float rotationY;
    float scale;
    int textureIndex; // 0, 1, or 2 for your three textures
};

class Forest {
public:
    Forest(Drawable* model, GLuint shaderID, int count);
    ~Forest();

    // Set terrain bounds for placement
    void setTerrainBounds(float minX, float maxX, float minZ, float maxZ,
        float minY, float maxY, float scale);

    // Load terrain data
    bool loadTerrainBinary(const std::string& filePath);
    void loadHeightData(const std::vector<float>& heights, int resolution);
    bool isFarFromExistingTrees(float x, float z) const;
    // Generate tree positions
    void generate();

    // Render all trees
    void draw();
    std::vector<TreeInstance> instances; //sos

private:
    void setupInstancing();
    void uploadToGPU();
    float sampleHeight(float x, float z) const;
    float sampleMask(const std::vector<float>& mask, float x, float z) const;
    bool isValidPlacement(float x, float z, float y) const;

    Drawable* treeModel;
    GLuint shader;
    int targetInstanceCount;

    // Instance data
    //std::vector<TreeInstance> instances;
    std::vector<glm::mat4> modelMatrices;
    std::vector<int> textureIndices;

    // GPU buffers
    GLuint instanceVBO;
    GLuint textureVBO;

    // Terrain data
    std::vector<float> heightData;
    std::vector<float> lakeMask;
    //std::vector<float> mountainMask;
    std::vector<float> forestMask;
    int gridResolution;
    float scalingFactor;

    // Terrain bounds
    float minX, maxX, minZ, maxZ, minY, maxY;
};

#endif // FOREST_H