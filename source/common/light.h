#include <glm/glm.hpp>

class Light {
public:

    GLFWwindow* window;
    // Light parameters
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    glm::vec3 lightPosition_worldspace;
    glm::vec3 direction;
    glm::vec3 sun_pos;
	float radius;
    glm::vec4 La;
    glm::vec4 Ld;
    glm::vec4 Ls;


    static int chosen_light_id;



    float nearPlane;
    float farPlane;

    float lightSpeed;


    // Where the light will look at
    glm::vec3 targetPosition;

    // Constructor
    Light(GLFWwindow* window,
        glm::vec4 init_La,
        glm::vec4 init_Ld,
        glm::vec4 init_Ls,
        glm::vec3 init_direction,
        float radius);
  
    void update();

    glm::mat4 lightVP();
    void fitToCameraFrustum(const glm::mat4& cameraView, const glm::mat4& cameraProj);
    //void fitToCameraFrustum(const mat4& cameraView, const mat4& cameraProj);
};
