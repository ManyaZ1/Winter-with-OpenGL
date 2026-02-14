// common/snow.cpp
#include "snow.h"
#include <common/texture.h>
#include <random>
#include <iostream>
using namespace glm;

SnowSystem::SnowSystem(int max) : maxParticles(max), isSnowing(false) {
    particles.resize(maxParticles);
    
    spawnHeight = 30.0f; // Height where snow spawns (lowered from 100)
}

SnowSystem::~SnowSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

//void SnowSystem::initialize(GLuint program, GLuint texture) {
//    shaderProgram = program;
//
//    // Load snow texture (simple white circle/snowflake)
//    //snowTexture = texture; //loadSOIL("assets/snowflake.png");
//
//    initBuffers();
//}
void SnowSystem::initialize(GLuint program) {//, GLuint texture
    if (program == 0) {
        std::cerr << "ERROR: SnowSystem received invalid shader program (0)" << std::endl;
        return;
    }

    shaderProgram = program;

    // Load snow texture (simple white circle/snowflake)
    //snowTexture = texture; //loadSOIL("assets/snowflake.png");

    initBuffers();

    std::cout << "SnowSystem initialized with shader program: " << shaderProgram << std::endl;
}


void SnowSystem::initBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Allocate space for particle data
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(SnowParticle),
        nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
}

void SnowSystem::resetParticle(SnowParticle& p, glm::vec3 cameraPos) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> distX(-80.0f, 80.0f);
    static std::uniform_real_distribution<float> distZ(-80.0f, 80.0f);
    static std::uniform_real_distribution<float> distSize(0.5f, 1.2f);
    static std::uniform_real_distribution<float> distVel(2.0f, 5.0f);

    // Spawn around camera position
    p.position = glm::vec3(
        cameraPos.x + distX(gen),
        spawnHeight,
        cameraPos.z + distZ(gen)
    );

    // Falling velocity with slight horizontal drift
    p.velocity = glm::vec3(
        (distX(gen) * 0.02f),  // Slight X drift
        -distVel(gen),         // Falling speed
        (distZ(gen) * 0.02f)   // Slight Z drift
    );

    p.size = distSize(gen);
    p.lifetime = 0.0f;
}

void SnowSystem::update(float deltaTime, glm::vec3 cameraPos) {
    if (!isSnowing) return;

	// wind added later, modify velocity here
	//  p.velocity.x += windStrength * deltaTime;
    for (auto& p : particles) {
        // Update position
        p.position += p.velocity * deltaTime;
        p.lifetime += deltaTime;

        // Reset if particle goes too low or too far from camera
        if (p.position.y < 0.0f ||
            glm::distance(glm::vec2(p.position.x, p.position.z),
                glm::vec2(cameraPos.x, cameraPos.z)) > 100.0f) {
            resetParticle(p, cameraPos);
        }
    }
}
//void SnowSystem::update_velocity(float x_offset, float z_offset) {
//    if (!isSnowing) return;
//    for (auto& p : particles) {
//        p.velocity.x += x_offset*0.01;
//        p.velocity.z += z_offset*0.01;
//    }
//}
//improve physics
void SnowSystem::update_velocity(float targetWindX, float targetWindZ) { //add *deltaTime?
    float airResistance = 0.7f; // How quickly snow matches wind (0.0 to 1.0)

    for (auto& p : particles) {
        // Instead of += (which is infinite acceleration), 
        // we "nudge" the current velocity toward the target wind speed.
        p.velocity.x += (targetWindX - p.velocity.x) * airResistance ;
        p.velocity.z += (targetWindZ - p.velocity.z) * airResistance ;
    }
}
void SnowSystem::render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix) {
    if (!isSnowing) {
        //std::cout << "Snow render skipped - not snowing" << std::endl;
        return;
    }
    //std::cout << "Rendering " << particles.size() << " snow particles" << std::endl;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer

    glUseProgram(shaderProgram);

    // Get uniform locations for snow shader
    GLuint vpLocation = glGetUniformLocation(shaderProgram, "VP");

    // Upload uniforms
    glm::mat4 VP = projectionMatrix * viewMatrix;
    glUniformMatrix4fv(vpLocation, 1, GL_FALSE, &VP[0][0]);
    //Set M to Identity because p.position is already in World Space
    glm::mat4 IdentityM = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "M"), 1, GL_FALSE, &IdentityM[0][0]);

    // Enable point sprites
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SPRITE);

    // Prepare vertex data (positions AND sizes)
    struct VertexData {
        glm::vec3 position;
        float size;
    };

    std::vector<VertexData> vertexData;
    vertexData.reserve(particles.size());
    for (const auto& p : particles) {
        vertexData.push_back({ p.position, p.size });
    }

    // Upload to GPU
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(VertexData),
        vertexData.data(), GL_DYNAMIC_DRAW);

    // Set vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(sizeof(glm::vec3)));

    // Draw all particles as points
    glDrawArrays(GL_POINTS, 0, vertexData.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);

    glDisable(GL_POINT_SPRITE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SnowSystem::toggle() {
    isSnowing = !isSnowing;
    std::cout << "SnowSystem::toggle() - isSnowing = " << isSnowing << std::endl;
    if (isSnowing) {
        // Initialize all particles when starting
        glm::vec3 initPos(0, spawnHeight, 0);
        for (auto& p : particles) {
            resetParticle(p, initPos);
        }
        std::cout << "Initialized " << particles.size() << " particles" << std::endl;
    }
}
