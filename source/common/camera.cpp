#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"
#include <vector>

#define SCALING_FACTOR 200 //lab.cpp kai camera.cpp
using namespace glm;
extern float sampleHeightAt(float x, float z, const std::vector<float>& heightData, int gridResolution, float minX, float maxX, float minZ, float maxZ);

Camera::Camera(GLFWwindow* window) : window(window) {
    position = vec3(15, 7, -15);
    horizontalAngle = 3.14f;
    verticalAngle = 0.0f;
    FoV = 40.0f;
    speed = 6.0f;
    mouseSpeed = 0.001f;
    fovSpeed = 0.25f;
}

void Camera::update() {
    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    // Get mouse position
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    //fix error 
    if (width == 0 || height == 0) {
        // Exit early to prevent the glm::perspective assertion failure.
        // The main loop's glfwGetWindowAttrib check (if implemented) will also help,
        // but this protects the Camera class itself.
        return;
    }
    // Reset mouse position for next frame
    glfwSetCursorPos(window, width / 2, height / 2);

    // Task 5.1: simple camera movement that moves in +-z and +-x axes
    /*/
    // Move forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position -= vec3(0, 0, 1) * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position += vec3(0, 0, 1) * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += vec3(1, 0, 0) * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= vec3(1, 0, 0) * deltaTime * speed;
    }

    // Task 5.2: update view matrix so it always looks at the origin
    projectionMatrix = perspective(radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
    viewMatrix = lookAt(
        position,
        vec3(0, 0, 0),
        vec3(0, 1, 0)
    );
    //*/

    // Task 5.3: Compute new horizontal and vertical angles, given windows size
    //*/
    // and cursor position
    horizontalAngle += mouseSpeed * float(width / 2 - xPos);
    verticalAngle += mouseSpeed * float(height / 2 - yPos);

    // Task 5.4: right and up vectors of the camera coordinate system
    // Task 5.4: right and up vectors of the camera coordinate system
    // use spherical coordinates
    vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
	Camera::direction_f = direction; //for slide
    // Right vector
    vec3 right(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    vec3 up = cross(right, direction);

    // Task 5.5: update camera position using the direction/right vectors
    // Move forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position -= right * deltaTime * speed;
    }

    // Task 5.6: handle zoom in/out effects
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        if (radians(FoV) > 0.1 + radians(fovSpeed))
        FoV -= fovSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        if (radians(FoV) < 3.14 - radians(fovSpeed))
        FoV += fovSpeed;
    }

    // Task 5.7: construct projection and view matrices
    float aspectRatio = (float)width / (float)height;
    projectionMatrix = perspective(radians(FoV), aspectRatio, 0.1f, 400.0f);
    viewMatrix = lookAt(
        position,
        position + direction,
        up
    );
    //*/

    // Homework XX: perform orthographic projection

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}

// Helper function: Create reflection matrix for a plane
mat4 getReflectionMatrix(vec3 planePoint, vec3 planeNormal) {
    planeNormal = normalize(planeNormal);
    float d = -dot(planeNormal, planePoint);

    // Reflection matrix for plane: ax + by + cz + d = 0
    mat4 R = mat4(1.0f);  // Start with identity

    R[0][0] = 1.0f - 2.0f * planeNormal.x * planeNormal.x;
    R[1][0] = -2.0f * planeNormal.x * planeNormal.y;
    R[2][0] = -2.0f * planeNormal.x * planeNormal.z;
    R[3][0] = -2.0f * planeNormal.x * d;

    R[0][1] = -2.0f * planeNormal.y * planeNormal.x;
    R[1][1] = 1.0f - 2.0f * planeNormal.y * planeNormal.y;
    R[2][1] = -2.0f * planeNormal.y * planeNormal.z;
    R[3][1] = -2.0f * planeNormal.y * d;

    R[0][2] = -2.0f * planeNormal.z * planeNormal.x;
    R[1][2] = -2.0f * planeNormal.z * planeNormal.y;
    R[2][2] = 1.0f - 2.0f * planeNormal.z * planeNormal.z;
    R[3][2] = -2.0f * planeNormal.z * d;

    R[0][3] = 0.0f;
    R[1][3] = 0.0f;
    R[2][3] = 0.0f;
    R[3][3] = 1.0f;

    return R;
}

// Main reflection function
mat4 Camera::getReflectionViewMatrix(float waterHeight) {
    // Define water plane: point on plane and normal vector
    vec3 waterPlanePoint = vec3(0.0f, waterHeight, 0.0f);
    vec3 waterPlaneNormal = vec3(0.0f, 1.0f, 0.0f);  // Pointing up

    // Get reflection matrix for the water plane
    mat4 reflectionMatrix = getReflectionMatrix(waterPlanePoint, waterPlaneNormal);

    // Apply reflection to the INVERSE view matrix
    // (because view matrix transforms world->camera, we need camera->world first)
    mat4 inverseView = inverse(viewMatrix);
    mat4 mirroredInverseView = reflectionMatrix * inverseView;

    // Convert back to view matrix
    return inverse(mirroredInverseView);
}
