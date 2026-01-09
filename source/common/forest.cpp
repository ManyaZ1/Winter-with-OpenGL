#include "forest.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <iostream>
#include <fstream>
// Height constraints for tree placement
static constexpr float TREE_MIN_HEIGHT = 3.0f;//8.0f;
static constexpr float TREE_MAX_HEIGHT = 28.0f;// Raised from 28.0

static constexpr float MIN_TREE_TREE_DIST = 4.0f;   // tune this
static constexpr float MIN_TREE_BUSH_DIST = 2.0f; // tune this

//Forest::Forest(Drawable* model, GLuint shaderID, int count)
Forest::Forest(Drawable* model, GLuint shaderID, int count, float treeScale, int texMode)
    : treeModel(model), shader(shaderID), targetInstanceCount(count),
    defaultScale(treeScale), textureMode(texMode)
    //: treeModel(model), shader(shaderID), targetInstanceCount(count)
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

Forest::~Forest() {
    if (instanceVBO != 0) {
        glDeleteBuffers(1, &instanceVBO);
    }
    if (textureVBO != 0) {
        glDeleteBuffers(1, &textureVBO);
    }
}

void Forest::setTerrainBounds(float minX_, float maxX_, float minZ_, float maxZ_,
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

bool Forest::loadTerrainBinary(const std::string& filePath) {
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

//bool Forest::isFarFromExistingTrees(float x, float z) const {
//    for (const auto& t : instances) {
//        float dx = x - t.position.x;
//        float dz = z - t.position.z;
//        if (dx * dx + dz * dz < MIN_TREE_TREE_DIST * MIN_TREE_TREE_DIST)
//            return false;
//    }
//    return true;
//}

bool Forest::isFarFromExistingTrees(float x, float z) const {
    // Check against own trees
    for (const auto& t : instances) {
        float dx = x - t.position.x;
        float dz = z - t.position.z;
        if (dx * dx + dz * dz < MIN_TREE_TREE_DIST * MIN_TREE_TREE_DIST)
            return false;
    }
    // Check against the other forest's trees
    for (const auto& pos : externalObstacles) {
        float dx = x - pos.x;
        float dz = z - pos.z;
        if (dx * dx + dz * dz < MIN_TREE_TREE_DIST * MIN_TREE_TREE_DIST)
            return false;
    }
    return true;
}

void Forest::addExternalPositions(const std::vector<TreeInstance>& otherInstances) {
    for (const auto& t : otherInstances) {
        externalObstacles.push_back(t.position);
    }
}

void Forest::loadHeightData(const std::vector<float>& heights, int resolution) {
    heightData = heights;
    gridResolution = resolution;
}

//void Forest::loadLakeMask(const std::vector<float>& mask) {
//    lakeMask = mask;
//}
//
//void Forest::loadMountainMask(const std::vector<float>& mask) {
//    mountainMask = mask;
//}

float Forest::sampleHeight(float x, float z) const {
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

    return heightData[iz * gridResolution + ix] ;
}

float Forest::sampleMask(const std::vector<float>& mask, float x, float z) const {
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
bool Forest::isValidPlacement(float x, float z, float y) const {
    // Debug: print rejection reasons for first few attempts
    static int debugCount = 0;
    bool shouldDebug = 1;//(debugCount < 10);

    if (y <= TREE_MIN_HEIGHT) {
        if (shouldDebug) {
            std::cout << "Rejected: y=" << y << " < min=" << TREE_MIN_HEIGHT << std::endl;
            debugCount++;
        }
        return false;
    }

    if (y > TREE_MAX_HEIGHT) {
        if (shouldDebug) {
            std::cout << "Rejected: y=" << y << " > max=" << TREE_MAX_HEIGHT << std::endl;
            debugCount++;
        }
        return false;
    }

    float forestVal = sampleMask(forestMask, x, z);
    /*if (forestVal > 0.5f)
        return false;*/
    if (forestVal > 0.5f) {
        // We are OUTSIDE the mask (on a mountain/plain)
        // Introduce a random chance (e.g., 2% chance to keep the tree)
        float randomRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        if (randomRoll > 0.02f) { // 98% of trees outside the mask are rejected
            return false;
        }
        // "lucky" stray tree!
    }
    std::cout << "x=" << x
        << " z=" << z
        << " u=" << (x - minX) / (maxX - minX)
        << " v=" << (z - minZ) / (maxZ - minZ)
        << " mask=" << forestVal << std::endl;

    float lakeMaskVal = sampleMask(lakeMask, x, z);
    if (!lakeMask.empty() && lakeMaskVal > 0.5f) {
        if (shouldDebug) {
            std::cout << "Rejected: lake mask=" << lakeMaskVal << std::endl;
            debugCount++;
        }
        return false;
    }
/*
    float mountainMaskVal = sampleMask(mountainMask, x, z);*/
    /*if (!mountainMask.empty() && mountainMaskVal > 0.5f) {
        if (shouldDebug) {
            std::cout << "Rejected: mountain mask=" << mountainMaskVal << std::endl;
            debugCount++;
        }
        return false;
    }*/

    if (shouldDebug) {
        std::cout << "Accepted: x=" << x << " z=" << z << " y=" << y << std::endl;
        debugCount++;
    }

    return true;
}

//bool Forest::isValidPlacement(float x, float z, float y) const {
//    // Check height bounds
//    if (y < TREE_MIN_HEIGHT || y > TREE_MAX_HEIGHT)
//        return false;
//
//    // Check lake mask (avoid water)
//    if (!lakeMask.empty() && sampleMask(lakeMask, x, z) > 0.5f)
//        return false;
//
//    // Check mountain mask (avoid steep areas)
//    if (!mountainMask.empty() && sampleMask(mountainMask, x, z) > 0.5f)
//        return false;
//
//    return true;
//}

void Forest::generate() {
    std::mt19937 rng(1337); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> rx(minX, maxX);
    std::uniform_real_distribution<float> rz(minZ, maxZ);
    std::uniform_real_distribution<float> rs(0.4f, 0.7f); // Scale variation
    std::uniform_real_distribution<float> rr(0.0f, glm::two_pi<float>()); // Rotation
    std::uniform_int_distribution<int> rt(0, 2); // Texture index (0-2)

    instances.clear();
    modelMatrices.clear();
    textureIndices.clear();

    int attempts = 0;
    int maxAttempts = targetInstanceCount * 10; // More attempts

    std::cout << "Generating trees..." << std::endl;
    std::cout << "Height data available: " << !heightData.empty() << std::endl;
    std::cout << "Lake mask available: " << !lakeMask.empty() << std::endl;
   // std::cout << "Mountain mask available: " << !mountainMask.empty() << std::endl;
	std::cout << "Forest mask available: " << !forestMask.empty() << std::endl;

    while ((int)instances.size() < targetInstanceCount && attempts < maxAttempts) {
        //std::cout << (int)instances.size() << "attempts" << attempts << std::endl;
        attempts++;

        float x = rx(rng);
        float z = rz(rng);
        float y = sampleHeight(x, z);
        std::cout << "Tree " << instances.size()
            << " pos = (" << x << ", " << y << ", " << z << ")\n";
        if (!isValidPlacement(x, z, y))
            continue;
        if (!isFarFromExistingTrees(x, z))
            continue;
        TreeInstance t;
        t.position = glm::vec3(x, y, z);    
        t.rotationY = rr(rng);
        //t.scale = 0.4f;//2.0f + rs(rng);
        t.scale = defaultScale; // Use the scale passed in the constructor
        t.textureIndex = rt(rng);
        //t.scale = 0.00005f;
        instances.push_back(t);
        // Progress indicator
        if (instances.size() % 10 == 0) {
            std::cout << "Generated " << instances.size() << " trees..." << std::endl;
        }
    }

    std::cout << "Generated " << instances.size() << " trees (target: "
        << targetInstanceCount << ")" << std::endl;
	// add placing deer and bear here later
    
    if (instances.empty()) {
        std::cerr << "ERROR: No valid tree positions found!" << std::endl;
        std::cerr << "Check terrain bounds and height constraints" << std::endl;
        return;
    }
    // Build model matrices
    modelMatrices.reserve(instances.size());
    textureIndices.reserve(instances.size());

    for (const auto& t : instances) {
        glm::mat4 M(1.0f);
        M = glm::translate(M, t.position);
        M = glm::rotate(M, t.rotationY, glm::vec3(0, 1, 0));
        M = glm::scale(M, glm::vec3(t.scale));
        modelMatrices.push_back(M);
        textureIndices.push_back(t.textureIndex);
    }

    uploadToGPU();
}

void Forest::setupInstancing() {
    if (!treeModel || modelMatrices.empty()) return;

    // 1. Link the instance count to the drawable
    treeModel->instanceCount = (int)modelMatrices.size();

    // 2. Bind the tree VAO to record settings
    treeModel->bind();

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
void Forest::uploadToGPU() {
    setupInstancing();
}
void Forest::draw() {
    if (!treeModel || modelMatrices.empty()) return;
    glUniform1i(glGetUniformLocation(shader, "useTexture"), textureMode); //specific mode update!!
    treeModel->bind();

    // Re-enable instancing slots specifically for the forest draw
    for (int i = 4; i <= 8; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1); // Ensure divisor is 1 for instancing
    }

    glDrawElementsInstanced(
        GL_TRIANGLES,
        (GLsizei)treeModel->indices.size(),
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
