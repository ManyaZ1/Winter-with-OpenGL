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
             glm::vec3 init_direction) : window(window) {
    La = init_La;
    Ld = init_Ld;
    Ls = init_Ls;
    // Directional light → direction is what matters
    // Treat input as "sun position hint"
    lightPosition_worldspace = init_direction;

    // Direction points FROM sun TOWARDS scene
    direction = normalize(lightPosition_worldspace);
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
    projectionMatrix = ortho(
        -150.0f, 150.0f,
        -150.0f, 150.0f,
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

    // Place sun FAR away so it sits on sky dome
    float sunDistance = 120.0f;
    lightPosition_worldspace = sceneCenter - direction * sunDistance;

    // --- LIGHT VIEW MATRIX FOR SHADOWS ---
    glm::vec3 up = vec3(0, 1, 0);
    viewMatrix = lookAt(lightPosition_worldspace, sceneCenter, up);
    //viewMatrix = lookAt(vec3(0,4,5), sceneCenter, up);//lightPosition_worldspace
}


/*void Light::update() {


   // Move across z-axis
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        lightPosition_worldspace += lightSpeed * vec3(0.0, 0.0, 1.0);
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
        lightPosition_worldspace -= lightSpeed * vec3(0.0, 0.0, 1.0);
    }
    // Move across x-axis
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        lightPosition_worldspace += lightSpeed * vec3(1.0, 0.0, 0.0);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        lightPosition_worldspace -= lightSpeed * vec3(1.0, 0.0, 0.0);
    }
    // Move across y-axis
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        lightPosition_worldspace += lightSpeed * vec3(0.0, 1.0, 0.0);
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        lightPosition_worldspace -= lightSpeed * vec3(0.0, 1.0, 0.0);
    }
    


    // We have the direction of the light and the point where the light is looking at
    // We will use this information to calculate the "up" vector, 
    // just like we did with the camera

    direction = normalize(targetPosition - lightPosition_worldspace);


    // converting direction to cylidrical coordinates
    float x = direction.x;
    float y = direction.y;
    float z = direction.z;

    // We don't need to calculate the vertical angle
    
    float horizontalAngle;
    if (z > 0.0) horizontalAngle = atan(x/z);
    else if (z < 0.0) horizontalAngle = atan(x/z) + 3.1415f;
    else horizontalAngle = 3.1415f / 2.0f;

    // Right vector
    vec3 right(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    vec3 up = cross(right, direction);
   
    viewMatrix = lookAt(
        lightPosition_worldspace,
        targetPosition,
        up 
    );
    //

}*/


mat4 Light::lightVP() {
    return projectionMatrix * viewMatrix;
}