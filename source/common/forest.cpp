#include "forest.h"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <iostream>
#include <fstream>
// Height constraints for tree placement
static constexpr float TREE_MIN_HEIGHT = 3.0f;//8.0f;
static constexpr float TREE_MAX_HEIGHT = 3800.0f;// Raised from 28.0

Forest::Forest(Drawable* model, GLuint shaderID, int count)
    : treeModel(model), shader(shaderID), targetInstanceCount(count)
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
    gridResolution = 512;
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
        // 1. Read Header (matching Python's "ifffffff")
        // Order: res, scale, minX, maxX, minZ, maxZ, min_h, max_h
        file.read(reinterpret_cast<char*>(&gridResolution), sizeof(int));
        file.read(reinterpret_cast<char*>(&scalingFactor), sizeof(float));
        file.read(reinterpret_cast<char*>(&minX), sizeof(float));
        file.read(reinterpret_cast<char*>(&maxX), sizeof(float));
        file.read(reinterpret_cast<char*>(&minZ), sizeof(float));
        file.read(reinterpret_cast<char*>(&maxZ), sizeof(float));
        file.read(reinterpret_cast<char*>(&minY), sizeof(float));
        file.read(reinterpret_cast<char*>(&maxY), sizeof(float));

        // 2. Prepare memory buffers
        size_t numElements = static_cast<size_t>(gridResolution) * gridResolution;
        heightData.assign(numElements, 0.0f);
        lakeMask.assign(numElements, 0.0f);
        mountainMask.assign(numElements, 0.0f);

        // 3. Read raw float buffers sequentially
        // Read Heightmap
        file.read(reinterpret_cast<char*>(heightData.data()), numElements * sizeof(float));

        // Read Lake Mask
        file.read(reinterpret_cast<char*>(lakeMask.data()), numElements * sizeof(float));

        // Read Mountain Mask
        file.read(reinterpret_cast<char*>(mountainMask.data()), numElements * sizeof(float));

        if (file.fail()) {
            throw std::runtime_error("File stream failed during data read. File might be truncated.");
        }

        std::cout << "Successfully loaded terrain: " << gridResolution << "x" << gridResolution << std::endl;
        std::cout << "Bounds X: [" << minX << ", " << maxX << "] Y: [" << minY << ", " << maxY << "]" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Exception while loading terrain: " << e.what() << std::endl;
        file.close();
        return false;
    }

    file.close();
    return true;
}

void Forest::loadHeightData(const std::vector<float>& heights, int resolution) {
    heightData = heights;
    gridResolution = resolution;
}

void Forest::loadLakeMask(const std::vector<float>& mask) {
    lakeMask = mask;
}

void Forest::loadMountainMask(const std::vector<float>& mask) {
    mountainMask = mask;
}

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
    bool shouldDebug = (debugCount < 10);

    if (y < TREE_MIN_HEIGHT) {
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

    float lakeMaskVal = sampleMask(lakeMask, x, z);
    if (!lakeMask.empty() && lakeMaskVal > 0.5f) {
        if (shouldDebug) {
            std::cout << "Rejected: lake mask=" << lakeMaskVal << std::endl;
            debugCount++;
        }
        return false;
    }

    float mountainMaskVal = sampleMask(mountainMask, x, z);
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
    std::cout << "Mountain mask available: " << !mountainMask.empty() << std::endl;
    
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

        TreeInstance t;
        t.position = glm::vec3(x, y, z);    
        t.rotationY = rr(rng);
        t.scale = rs(rng);
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
//
//void Forest::setupInstancing() {
//    if (!treeModel) {
//        std::cerr << "Error: treeModel is null!" << std::endl;
//        return;
//    }
//
//    // Bind the tree model's VAO
//    treeModel->bind();
//
//    // Create and upload model matrix buffer
//    glGenBuffers(1, &instanceVBO);
//    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
//    glBufferData(
//        GL_ARRAY_BUFFER,
//        modelMatrices.size() * sizeof(glm::mat4),
//        modelMatrices.data(),
//        GL_STATIC_DRAW
//    );
//
//    // Set up mat4 as 4 vec4 attributes (locations 4-7)
//    // These need to be consecutive and match your vertex shader
//    for (int i = 0; i < 4; i++) {
//        glEnableVertexAttribArray(4 + i);
//        glVertexAttribPointer(
//            4 + i,                          // attribute location
//            4,                               // size (vec4)
//            GL_FLOAT,                        // type
//            GL_FALSE,                        // normalized?
//            sizeof(glm::mat4),              // stride
//            (void*)(sizeof(float) * i * 4)  // offset
//        );
//        glVertexAttribDivisor(4 + i, 1);    // advance once per instance
//    }
//
//    // Create and upload texture index buffer
//    glGenBuffers(1, &textureVBO);
//    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
//    glBufferData(
//        GL_ARRAY_BUFFER,
//        textureIndices.size() * sizeof(int),
//        textureIndices.data(),
//        GL_STATIC_DRAW
//    );
//
//    // Set up texture index attribute (location 8)
//    glEnableVertexAttribArray(8);
//    glVertexAttribIPointer(8, 1, GL_INT, sizeof(int), (void*)0);
//    glVertexAttribDivisor(8, 1);
//
//    // Unbind
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//    std::cout << "Instancing set up for " << modelMatrices.size() << " trees" << std::endl;
//}
//void Forest::setupInstancing() {
//    if (!treeModel) {
//        std::cerr << "Error: treeModel is null!" << std::endl;
//        return;
//    }
//
//    std::cout << "Setting up instancing for " << modelMatrices.size() << " trees..." << std::endl;
//
//    // CRITICAL: Bind the tree model's VAO FIRST
//    treeModel->bind();
//
//    // Clean up old buffers if they exist
//    if (instanceVBO != 0) {
//        glDeleteBuffers(1, &instanceVBO);
//    }
//    if (textureVBO != 0) {
//        glDeleteBuffers(1, &textureVBO);
//    }
//
//    // Create and upload model matrix buffer
//    glGenBuffers(1, &instanceVBO);
//    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
//    glBufferData(
//        GL_ARRAY_BUFFER,
//        modelMatrices.size() * sizeof(glm::mat4),
//        modelMatrices.data(),
//        GL_STATIC_DRAW
//    );
//
//    // Set up mat4 as 4 vec4 attributes (locations 4-7)
//    for (int i = 0; i < 4; i++) {
//        glEnableVertexAttribArray(4 + i);
//        glVertexAttribPointer(
//            4 + i,
//            4,
//            GL_FLOAT,
//            GL_FALSE,
//            sizeof(glm::mat4),
//            (void*)(sizeof(float) * i * 4)
//        );
//        glVertexAttribDivisor(4 + i, 1);
//    }
//
//    // Create and upload texture index buffer
//    glGenBuffers(1, &textureVBO);
//    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
//    glBufferData(
//        GL_ARRAY_BUFFER,
//        textureIndices.size() * sizeof(int),
//        textureIndices.data(),
//        GL_STATIC_DRAW
//    );
//
//    glEnableVertexAttribArray(8);
//    glVertexAttribIPointer(8, 1, GL_INT, sizeof(int), (void*)0);
//    glVertexAttribDivisor(8, 1);
//    // Set divisors
//    for (int i = 0; i < 4; i++) {
//        glEnableVertexAttribArray(4 + i);
//        glVertexAttribDivisor(4 + i, 1);
//    }
//    glEnableVertexAttribArray(8);
//    glVertexAttribDivisor(8, 1);
//
//    // CRITICAL: Unbind the buffers FIRST
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//    // CRITICAL: Now unbind the VAO. 
//    // DO NOT disable the attributes here if you want the VAO to remember them.
//    // VAOs are meant to store the "Enabled" state of attributes.
//    glBindVertexArray(0);
//    //// CRITICAL: Unbind everything to prevent state leakage
//    //glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//    //// CRITICAL: Disable the instancing attributes immediately after setup
//    //// They will be re-enabled when forest->draw() is called
//    //for (int i = 4; i <= 8; i++) {
//    //    glDisableVertexAttribArray(i);
//    //}
//
//    //// CRITICAL: Unbind the VAO
//    //glBindVertexArray(0);
//
//    //std::cout << "Instancing setup complete. Attributes disabled and VAO unbound." << std::endl;
//
//    ////debuggg
//    //glBindVertexArray(0); // unbind VAO
//    //for (int i = 4; i <= 8; i++) {
//    //    glDisableVertexAttribArray(i);   // disable instancing
//    //    if (i <= 7) glVertexAttribDivisor(i, 0); // reset divisors
//    //}
//
//}
void Forest::setupInstancing() {
    if (!treeModel || modelMatrices.empty()) {
        std::cerr << "Error: treeModel is null or no instances to setup!" << std::endl;
        return;
    }

    // 1. Bind the tree model's VAO to record these settings
    treeModel->bind();

    // Clean up old buffers if they exist (prevents memory leaks on re-generation)
    if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
    if (textureVBO != 0) glDeleteBuffers(1, &textureVBO);

    // 2. Setup Model Matrices (Attributes 4, 5, 6, 7)
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        modelMatrices.size() * sizeof(glm::mat4),
        modelMatrices.data(),
        GL_STATIC_DRAW
    );

    // A mat4 takes up 4 attribute slots
    for (int i = 0; i < 4; i++) {
        GLuint loc = 4 + i;
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(
            loc,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(glm::mat4),
            (void*)(sizeof(float) * i * 4)
        );
        glVertexAttribDivisor(loc, 1); // Advance once per instance, not per vertex
    }

    // 3. Setup Texture Indices (Attribute 8)
    glGenBuffers(1, &textureVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        textureIndices.size() * sizeof(int),
        textureIndices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(8);
    glVertexAttribIPointer(8, 1, GL_INT, sizeof(int), (void*)0);
    glVertexAttribDivisor(8, 1); // Advance once per instance

    // 4. CLEANUP (The order here is vital)

    // Unbind the buffer first
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Disable the attributes specifically for this VAO so they don't 
    // stay active when you use the VAO for other things (like shadow passes)
    // They will be re-enabled inside your Forest::draw() call.
    for (int i = 4; i <= 8; i++) {
        glDisableVertexAttribArray(i);
    }

    // Finally, unbind the VAO
    glBindVertexArray(0);

    std::cout << "Instancing successfully set up for " << modelMatrices.size() << " trees." << std::endl;
}
void Forest::uploadToGPU() {
    setupInstancing();
}

void Forest::draw() {
    if (!treeModel || modelMatrices.empty()) {
        return;
    }

    // Bind the tree model's VAO
    treeModel->bind();

    // Draw all instances
    glDrawElementsInstanced(
        GL_TRIANGLES,
        (GLsizei)treeModel->indices.size(), // Use the indices vector size
        GL_UNSIGNED_INT,
        0,
        (GLsizei)modelMatrices.size()
    );
    // FIX: Disable the attributes so they don't leak to other models
    for (int i = 0; i < 5; i++) { // Locations 4, 5, 6, 7, and 8
        glDisableVertexAttribArray(4 + i);
    }
}
