#include "bush.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <iostream>
#include <fstream>
static constexpr float BUSH_MIN_HEIGHT = 3.0f;
static constexpr float BUSH_MAX_HEIGHT = 28.0f;
static constexpr float MIN_TREE_BUSH_DIST = 2.5f;
static constexpr float MIN_BUSH_BUSH_DIST = 1.2f;


BushField::BushField(Drawable* model, GLuint shaderID, int count)
    : bushModel(model), shader(shaderID), targetInstanceCount(count)
{
    instanceVBO = 0;
    textureVBO = 0;

    // Initialize terrain bounds to reasonable defaults
    minX = -100.0f;
    maxX = 100.0f;
    minZ = -100.0f;
    maxZ = 100.0f;
    minY = 0.0f;
    maxY = 50.0f;
    gridResolution = 1024;
    scalingFactor = 200.0f; // Match SCALING_FACTOR from winter.cpp
}

BushField::~BushField() {
    if (instanceVBO != 0) {
        glDeleteBuffers(1, &instanceVBO);
    }
    if (textureVBO != 0) {
        glDeleteBuffers(1, &textureVBO);
    }
}
bool BushField::isFarFromExistingBush(float x, float z) const {
    for (const auto& t : instances) {
        float dx = x - t.position.x;
        float dz = z - t.position.z;
        if (dx * dx + dz * dz < MIN_BUSH_BUSH_DIST * MIN_BUSH_BUSH_DIST)
            return false;
    }
    return true;
}
bool BushField::isFarFromTrees(float x, float z) const {
    for (const auto& t : treePositions) {
        float dx = x - t.x;
        float dz = z - t.z;
        if (dx * dx + dz * dz < MIN_TREE_BUSH_DIST * MIN_TREE_BUSH_DIST)
            return false;
    }
    return true;
}
void BushField::setTreeReferences(const std::vector<glm::vec3>& treePositions_)
{
    treePositions = treePositions_;
}

void BushField::setTerrainBounds(float minX_, float maxX_, float minZ_, float maxZ_,
    float minY_, float maxY_, float scale) {
    minX = minX_;
    maxX = maxX_;
    minZ = minZ_;
    maxZ = maxZ_;
    minY = minY_;
    maxY = maxY_;
    scalingFactor = scale;
    std::cout << "Forest bounds set - X: [" << minX << ", " << maxX
        << "] Z: [" << minZ << ", " << maxZ
        << "] Y: [" << minY << ", " << maxY << "]" << std::endl;
}


bool BushField::loadTerrainBinary(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open terrain file: " << filePath << std::endl;
        return false;
    }
    try {
        // 1. Read Header (Matches Python "ifffffff")
        file.read(reinterpret_cast<char*>(&gridResolution), sizeof(int));
        file.read(reinterpret_cast<char*>(&scalingFactor), sizeof(float));
        file.read(reinterpret_cast<char*>(&minX), sizeof(float));
        file.read(reinterpret_cast<char*>(&maxX), sizeof(float));
        file.read(reinterpret_cast<char*>(&minZ), sizeof(float));
        file.read(reinterpret_cast<char*>(&maxZ), sizeof(float));
        file.read(reinterpret_cast<char*>(&minY), sizeof(float));
        file.read(reinterpret_cast<char*>(&maxY), sizeof(float));

        size_t numElements = static_cast<size_t>(gridResolution) * gridResolution;
        heightData.assign(numElements, 0.0f);
        forestMask.assign(numElements, 0.0f);
        lakeMask.assign(numElements, 0.0f);

        // 2. Read raw float buffers SEQUENTIALLY (Must match Python order!)
        file.read(reinterpret_cast<char*>(heightData.data()), numElements * sizeof(float)); // Block 1
        file.read(reinterpret_cast<char*>(forestMask.data()), numElements * sizeof(float)); // Block 2
        file.read(reinterpret_cast<char*>(lakeMask.data()), numElements * sizeof(float)); // Block 3

        if (file.fail()) throw std::runtime_error("File read failed.");
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        file.close();
        return false;
    }
    file.close();
    return true;
}

// Update this in bush.cpp
bool BushField::isValidPlacement(float x, float z, float y) const {
    if (y < BUSH_MIN_HEIGHT || y > BUSH_MAX_HEIGHT)
        return false;

    if (!lakeMask.empty() && sampleMask(lakeMask, x, z) > 0.5f)
        return false;

    // MATCH THE FOREST LOGIC: 
    // If trees are placed where mask < 0.5, bushes should be too.
    if (!forestMask.empty() && sampleMask(forestMask, x, z) > 0.5f)
        return false;

    return true;
}

//bool BushField::isValidPlacement(float x, float z, float y) const {
//    if (y < BUSH_MIN_HEIGHT || y > BUSH_MAX_HEIGHT)
//        return false;
//
//    if (!lakeMask.empty() && sampleMask(lakeMask, x, z) > 0.5f)
//        return false;
//    if (!forestMask.empty() && sampleMask(forestMask, x, z) < 0.5f)
//        return false;
//    return true;
//}

float BushField::sampleHeight(float x, float z) const {
    if (heightData.empty()) {
        return 0.0f; // Return ground level if no height data
    }

    // Convert world coords to normalized [0,1]
    float u = (x - minX) / (maxX - minX);
    float v = (z - minZ) / (maxZ - minZ);

    u = glm::clamp(u, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);

    // Convert to grid indices
    int ix = int(u * (gridResolution - 1));
    int iz = int(v * (gridResolution - 1));

    ix = glm::clamp(ix, 0, gridResolution - 1);
    iz = glm::clamp(iz, 0, gridResolution - 1);

    return heightData[iz * gridResolution + ix];
}

float BushField::sampleMask(const std::vector<float>& mask, float x, float z) const {
    if (mask.empty()) {
        return 0.0f; // No mask = valid placement
    }

    float u = (x - minX) / (maxX - minX);
    float v = (z - minZ) / (maxZ - minZ);

    u = glm::clamp(u, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);

    int ix = int(u * (gridResolution - 1));
    int iz = int(v * (gridResolution - 1));

    ix = glm::clamp(ix, 0, gridResolution - 1);
    iz = glm::clamp(iz, 0, gridResolution - 1);

    return mask[iz * gridResolution + ix];
}
void BushField::generate() {
    std::mt19937 rng(4242);
    std::uniform_real_distribution<float> rx(minX, maxX);
    std::uniform_real_distribution<float> rz(minZ, maxZ);
    std::uniform_real_distribution<float> rs(0.3f, 0.6f);
    std::uniform_real_distribution<float> rr(0.0f, glm::two_pi<float>());
    std::uniform_int_distribution<int> rt(0, 2);

    instances.clear();
    modelMatrices.clear();
    textureIndices.clear();

    int attempts = 0;
    int maxAttempts = targetInstanceCount * 10;

    while ((int)instances.size() < targetInstanceCount && attempts < maxAttempts) {
        attempts++;

        float x = rx(rng);
        float z = rz(rng);
        float y = sampleHeight(x, z);

        if (!isValidPlacement(x, z, y)) continue;
        if (!isFarFromTrees(x, z)) continue;
        if (!isFarFromExistingBush(x, z)) continue;

        BushInstance b;
        b.position = { x, y, z };
        b.rotationY = rr(rng);
        b.scale = 1.0 + rs(rng);
        b.textureIndex = rt(rng);

        instances.push_back(b);
    }

    for (const auto& b : instances) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, b.position);
        M = glm::rotate(M, b.rotationY, { 0,1,0 });
        M = glm::scale(M, glm::vec3(0.03f * b.scale, 0.03f * b.scale, 0.03f * b.scale));
        modelMatrices.push_back(M);
        textureIndices.push_back(b.textureIndex);
    }

    // reuse SAME instancing setup logic as Forest
    uploadToGPU();
}

void BushField::setupInstancing() {
    if (!bushModel || modelMatrices.empty()) return;

    // 1. Link the instance count to the drawable
    bushModel->instanceCount = (int)modelMatrices.size();

    // 2. Bind the tree VAO to record settings
    bushModel->bind();

    // Clean up old buffers if they exist
    if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
    if (textureVBO != 0) glDeleteBuffers(1, &textureVBO);

    // 3. Upload Model Matrices (Attributes 4-7)
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data(), GL_STATIC_DRAW);

    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(float) * i * 4));
        glVertexAttribDivisor(4 + i, 1);
    }

    // 4. Upload Texture Indices (Attribute 8)
    glGenBuffers(1, &textureVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, textureIndices.size() * sizeof(int), textureIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(8);
    glVertexAttribIPointer(8, 1, GL_INT, sizeof(int), (void*)0);
    glVertexAttribDivisor(8, 1);

    // 5. CRITICAL: RESET STATE
    // Unbind the buffer first
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Disable these attributes in the VAO so they don't leak 
    // to standard draws. They will be re-enabled in Forest::draw()
    for (int i = 4; i <= 8; i++) {
        glDisableVertexAttribArray(i);
    }

    // 6. Unbind the VAO last to close the state recording
    glBindVertexArray(0);
}
void BushField::uploadToGPU() {
    setupInstancing();
}
void BushField::draw() {
    if (!bushModel || modelMatrices.empty()) return;

    bushModel->bind();

    // Re-enable instancing slots specifically for the forest draw
    for (int i = 4; i <= 8; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1); // Ensure divisor is 1 for instancing
    }

    glDrawElementsInstanced(
        GL_TRIANGLES,
        (GLsizei)bushModel->indices.size(),
        GL_UNSIGNED_INT,
        0,
        (GLsizei)modelMatrices.size()
    );

    // Clean up immediately after drawing to prevent leakage
    for (int i = 4; i <= 8; i++) {
        glDisableVertexAttribArray(i);
        glVertexAttribDivisor(i, 0);
    }

    glBindVertexArray(0);
}