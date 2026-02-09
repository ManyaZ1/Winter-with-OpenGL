#include <glfw3.h>
#include <iostream>
#include <math.h>
#include <glm/gtc/matrix_transform.hpp>
#include "light.h"
#include <glm/gtx/transform.hpp>

using namespace glm;

int Light::chosen_light_id = 1;

// helper
glm::vec3 rotateAroundAxis(const glm::vec3& v, float angle, const glm::vec3& axis)
{
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, axis);
    return glm::vec3(R * glm::vec4(v, 0.0f));
}


Light::Light(GLFWwindow* window, 
             glm::vec4 init_La,
             glm::vec4 init_Ld,
             glm::vec4 init_Ls,
             glm::vec3 init_direction,
             float radius_in) : window(window) {
    La = init_La;
    Ld = init_Ld;
    Ls = init_Ls;
    nearPlane = 1.0f;
    farPlane = 300.0f;
    radius		= radius_in;
    // Directional light → direction is what matters
    // Treat input as "sun position hint"
    lightPosition_worldspace = init_direction;

    // Direction points FROM sun TOWARDS scene
    direction = normalize(init_direction); 
	sun_pos = -direction * radius;
    //lightPosition_worldspace = init_position;

    // setting near and far plane affects the detail of the shadow
    nearPlane = 1.0f;
    farPlane = 300.0f;
    // for point light
    //direction = normalize(targetPosition - lightPosition_worldspace); 

    lightSpeed = 0.02f;
    targetPosition = glm::vec3(0.0, 0.0, -5.0);


    //projectionMatrix = ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
    // Orthographic projection for directional light

    float orthoSize = 80.0f;
    // <============================Directional light frustum fitting=================================>

    /*Take the camera frustum corners

Transform them into light space

Compute a tight AABB

Build the orthographic projection from that*/

    projectionMatrix = ortho(
        -orthoSize, orthoSize,
        -orthoSize, orthoSize,
        nearPlane, farPlane
    );
}

void Light::update()
{
    // Rotate sun direction (like moving the sun in the sky)
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        direction = normalize(rotateAroundAxis(direction, lightSpeed, vec3(0, 1, 0)));

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        direction = normalize(rotateAroundAxis(direction, -lightSpeed, vec3(0, 1, 0)));

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        direction = normalize(rotateAroundAxis(direction, lightSpeed, vec3(1, 0, 0)));

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        direction = normalize(rotateAroundAxis(direction, -lightSpeed, vec3(1, 0, 0)));

    // --- SUN POSITION (FOR VISUALIZATION ONLY) ---
    glm::vec3 sceneCenter = glm::vec3(0.0f, 0.0f, -5.0f);
    sun_pos = - direction * radius;
    //// Place sun FAR away so it sits on sky dome
    //float sunDistance = 120.0f;
    //lightPosition_worldspace = sceneCenter - direction * sunDistance;

    //
    
    lightPosition_worldspace = sceneCenter - direction * 150.0f;
    // --- LIGHT VIEW MATRIX FOR SHADOWS ---
    //glm::vec3 up = vec3(0, 1, 0);
    glm::vec3 up = (glm::abs(glm::dot(direction, vec3(0, 1, 0))) > 0.99f) ? vec3(0, 0, 1) : vec3(0, 1, 0);
    viewMatrix = lookAt(lightPosition_worldspace, sceneCenter, up);
    //viewMatrix = lookAt(vec3(0,4,5), sceneCenter, up);//lightPosition_worldspace
}




mat4 Light::lightVP() {
    return projectionMatrix * viewMatrix;
}

void Light::fitToCameraFrustum(const mat4& cameraView, const mat4& cameraProj)
{
    // Get the 8 corners of the camera frustum in world space
    mat4 invCam = inverse(cameraProj * cameraView);

    vec4 frustumCornersWorldSpace[8];
    int i = 0;
    for (int x = 0; x < 2; ++x)
        for (int y = 0; y < 2; ++y)
            for (int z = 0; z < 2; ++z)
            {
                vec4 ndc(
                    x ? 1.0f : -1.0f,
                    y ? 1.0f : -1.0f,
                    z ? 1.0f : -1.0f,
                    1.0f
                );

                vec4 world = invCam * ndc;
                world /= world.w;
                frustumCornersWorldSpace[i++] = world;
            }

    // Calculate frustum center in world space
    vec3 frustumCenter(0.0f);
    for (int j = 0; j < 8; ++j)
    {
        frustumCenter += vec3(frustumCornersWorldSpace[j]);
    }
    frustumCenter /= 8.0f;

    // Update light view to look at frustum center
    glm::vec3 up = (glm::abs(glm::dot(direction, vec3(0, 1, 0))) > 0.99f) ? vec3(0, 0, 1) : vec3(0, 1, 0);

    // Position light far from frustum center along direction
    lightPosition_worldspace = frustumCenter - direction * 150.0f;
    viewMatrix = lookAt(lightPosition_worldspace, frustumCenter, up);

    // Transform corners to light view space
    float minX = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    for (int j = 0; j < 8; ++j)
    {
        vec4 ls = viewMatrix * frustumCornersWorldSpace[j];

        minX = std::min(minX, ls.x);
        maxX = std::max(maxX, ls.x);
        minY = std::min(minY, ls.y);
        maxY = std::max(maxY, ls.y);
        minZ = std::min(minZ, ls.z);
        maxZ = std::max(maxZ, ls.z);
    }

    // Generous padding
    const float padding = 30.0f;
    minX -= padding;
    maxX += padding;
    minY -= padding; 
    maxY += padding;
    minZ -= padding * 2.0f;  // More padding behind camera
    maxZ += padding;

    nearPlane = -maxZ;
    farPlane = -minZ;

    projectionMatrix = ortho(minX, maxX, minY, maxY, nearPlane, farPlane);
}
