// Include C++ headers
#include <iostream>
#include <string>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <common/model.h>
#include <common/texture.h>
#include <common/light.h> 


#include <vector>

#define SCALING_FACTOR 60 //lab.cpp kai camera.cpp




using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();

#define W_WIDTH  1800
#define W_HEIGHT  900
#define TITLE "Winter"

#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024




// Creating a structure to store the material parameters of an object
struct Material
{
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	float Ns;
};

// Global Variables
GLFWwindow* window;
Camera* camera;
Light* light;
Light* light2;
GLuint shaderProgram, depthProgram, miniMapProgram;
Drawable* model1;
Drawable* sphere;
Drawable* terrain;
Drawable* plane;
GLuint modelDiffuseTexture, modelSpecularTexture;
GLuint depthFBO, depthTexture;
GLuint depthFBO2, depthTexture2;

// Global instance to hold your terrain data after loading

Drawable* quad;

// locations for shaderProgram
GLuint scaling_factor_location;
GLuint viewMatrixLocation;
GLuint projectionMatrixLocation;
GLuint modelMatrixLocation;
GLuint KaLocation, KdLocation, KsLocation, NsLocation;
GLuint LaLocation, LdLocation, LsLocation;
GLuint LaLocation2, LdLocation2, LsLocation2;
GLuint lightPositionLocation;
GLuint lightPositionLocation2;
GLuint lightPowerLocation;
GLuint diffuseColorSampler;
GLuint specularColorSampler;
GLuint useTextureLocation;
GLuint depthMapSampler;
GLuint depthMapSampler2;
GLuint lightVPLocation;
GLuint light2VPLocation;


//scale textures
GLuint uvScaleLocation;



// locations for depthProgram
GLuint shadowViewProjectionLocation;
GLuint shadowModelLocation;
//GLuint shadowViewProjectionLocation2;

GLuint terrainTexture ;
GLuint terrainTexture2 ;
GLuint waterTexture ;
GLuint waterTexture2 ;
GLuint bottomTexture ;
GLuint maskTexture;
GLuint sunTexture;
GLuint skyTexture;
// locations for miniMapProgram
GLuint quadTextureSamplerLocation;

GLuint normDirLocation;

// Create two sample materials
const Material polishedSilver
{
	vec4{0.23125, 0.23125, 0.23125, 1},
	vec4{0.2775, 0.2775, 0.2775, 1},
	vec4{0.773911, 0.773911, 0.773911, 1},
	89.6f
};

const Material turquoise
{
	vec4{ 0.1, 0.18725, 0.1745, 0.8 },
	vec4{ 0.396, 0.74151, 0.69102, 0.8 },
	vec4{ 0.297254, 0.30829, 0.306678, 0.8 },
	12.8f
};

// NOTE: Since the Light and Material struct are used in the shader programs as well 
//		 it is recommended to create a function that will update all the parameters 
//       of an object.
// 
// Creating a function to upload (make uniform) the light parameters to the shader program
void uploadLight(const Light& light) {
	glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
	glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
	glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
	glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
}
void uploadLight(const Light& light, const Light& light2) {
	glUniform4f(LaLocation, light.La.r, light.La.g, light.La.b, light.La.a);
	glUniform4f(LdLocation, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
	glUniform4f(LsLocation, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
	glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);
	//upload for light 2
	glUniform4f(LaLocation2, light2.La.r, light2.La.g, light2.La.b, light2.La.a);
	glUniform4f(LdLocation2, light2.Ld.r, light2.Ld.g, light2.Ld.b, light2.Ld.a);
	glUniform4f(LsLocation2, light2.Ls.r, light2.Ls.g, light2.Ls.b, light2.Ls.a);
	glUniform3f(lightPositionLocation2, light2.lightPosition_worldspace.x,
		light2.lightPosition_worldspace.y, light2.lightPosition_worldspace.z);
}


// Creating a function to upload the material parameters of a model to the shader program
void uploadMaterial(const Material& mtl) {
	glUniform4f(KaLocation, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
	glUniform4f(KdLocation, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
	glUniform4f(KsLocation, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
	glUniform1f(NsLocation, mtl.Ns);
}

void createDepthFBOAndTexture(GLuint& fboID, GLuint& textureID) {
	// 1. Generate FBO
	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_FRAMEBUFFER, fboID);

	// 2. Generate Depth Texture
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture data/format (GL_DEPTH_COMPONENT)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Task 4.5: Wrapping to GL_CLAMP_TO_BORDER with a border color
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// 3. Attach Texture to FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureID, 0);

	// 4. Configure FBO (No color buffer needed for depth map)
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// 5. Check Status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		// NOTE: In a real app, you might just return an error code instead of terminating here.
		throw std::runtime_error("Frame buffer not initialized correctly");
	}

	// Unbind the FBO before exiting the function
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void free() {
	// Delete Shader Programs
	glDeleteProgram(shaderProgram);
	glDeleteProgram(depthProgram);
	glDeleteProgram(miniMapProgram);

	glfwTerminate();
}
void createContext() {
	// Create and compile our GLSL program from the shader
	shaderProgram = loadShaders("ShadowMapping.vertexshader", "ShadowMapping.fragmentshader");

	// Task 3.1 
	// Create and load the shader program for the depth buffer construction
	// You need to load and use the Depth.vertexshader, Depth.fragmentshader
	depthProgram = loadShaders("Depth.vertexshader", "Depth.fragmentshader");


	// Task 2.1
	// Use the MiniMap.vertexshader, "MiniMap.fragmentshader"
	miniMapProgram = loadShaders("MiniMap.vertexshader", "MiniMap.fragmentshader");


	// NOTE: Don't forget to delete the shader programs on the free() function

	// Get pointers to uniforms
	// --- shaderProgram ---
	projectionMatrixLocation = glGetUniformLocation(shaderProgram, "P");
	viewMatrixLocation = glGetUniformLocation(shaderProgram, "V");
	modelMatrixLocation = glGetUniformLocation(shaderProgram, "M");
	// for phong lighting
	KaLocation = glGetUniformLocation(shaderProgram, "mtl.Ka");
	KdLocation = glGetUniformLocation(shaderProgram, "mtl.Kd");
	KsLocation = glGetUniformLocation(shaderProgram, "mtl.Ks");
	NsLocation = glGetUniformLocation(shaderProgram, "mtl.Ns");
	LaLocation = glGetUniformLocation(shaderProgram, "light.La");
	LdLocation = glGetUniformLocation(shaderProgram, "light.Ld");
	LsLocation = glGetUniformLocation(shaderProgram, "light.Ls");
	lightPositionLocation = glGetUniformLocation(shaderProgram, "light.lightPosition_worldspace");
	lightPositionLocation2 = glGetUniformLocation(shaderProgram, "light2.lightPosition_worldspace");
	diffuseColorSampler = glGetUniformLocation(shaderProgram, "diffuseColorSampler");
	specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");
	scaling_factor_location= glGetUniformLocation(shaderProgram, "scaling_factor");
	uvScaleLocation = glGetUniformLocation(shaderProgram, "uvScale"); // <-- new
	LaLocation2 = glGetUniformLocation(shaderProgram, "light2.La");
	LdLocation2 = glGetUniformLocation(shaderProgram, "light2.Ld");
	LsLocation2 = glGetUniformLocation(shaderProgram, "light2.Ls");
	
	//hw 4
	normDirLocation = glGetUniformLocation(shaderProgram, "normDir");
	// Task 1.4
	useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

	// locations for shadow rendering
	depthMapSampler = glGetUniformLocation(shaderProgram, "shadowMapSampler");
	lightVPLocation = glGetUniformLocation(shaderProgram, "lightVP");
	light2VPLocation = glGetUniformLocation(shaderProgram, "light2VP");
	depthMapSampler2 = glGetUniformLocation(shaderProgram, "shadowMapSampler2");
	// --- depthProgram ---
	shadowViewProjectionLocation = glGetUniformLocation(depthProgram, "VP");
	shadowModelLocation = glGetUniformLocation(depthProgram, "M");
	//shadowViewProjectionLocation2 = glGetUniformLocation(depthProgram, "VP2"); //hw2
	// --- miniMapProgram ---
	quadTextureSamplerLocation = glGetUniformLocation(miniMapProgram, "textureSampler");

	// Loading a model
	// The terrain object from Gaea is loaded as terrain
	std::string modelPath = "assets/Mesher_LOD2.obj";
	terrain = new Drawable(modelPath);

	// Original objects are still needed for light visualization (model2) or removed entirely
	//model1 = new Drawable("suzanne.obj");
	// The textures for model1 are NOT loaded, as requested.
	// modelDiffuseTexture = loadSOIL("suzanne_diffuse.bmp");
	// modelSpecularTexture = loadSOIL("suzanne_specular.bmp");

	// model2 (sphere) is used for light visualization, keep loading it
	sphere = new Drawable("earth.obj");

	// Task 1.3
	// Creating a Drawable object using vertices, uvs, normals
	// In this task we will create a plane on which the shadows will be displayed

	

	// Task 2.2: Creating a 2D quad to visualize the depthmap
	// create geometry and vao for screen-space quad
	vector<vec3> quadVertices = {
	  vec3(0.5, 0.5, -1.0),
	  vec3(1.0, 0.5, -1.0),
	  vec3(1.0, 1.0, -1.0),
	  vec3(1.0, 1.0, -1.0),
	  vec3(0.5, 1.0, -1.0),
	  vec3(0.5, 0.5, -1.0)
	};

	vector<vec2> quadUVs = {
	  vec2(0.0, 0.0),
	  vec2(1.0, 0.0),
	  vec2(1.0, 1.0),
	  vec2(1.0, 1.0),
	  vec2(0.0, 1.0),
	  vec2(0.0, 0.0)
	};

	quad = new Drawable(quadVertices, quadUVs);
	createDepthFBOAndTexture(depthFBO, depthTexture);

	// Homework 2: create second depth FBO and texture
	createDepthFBOAndTexture(depthFBO2, depthTexture2);

	/* load textures */
	terrainTexture = loadSOIL("assets/aerial_rocks.bmp");
	glBindTexture(GL_TEXTURE_2D, terrainTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	 terrainTexture2 = loadSOIL("assets/grass2.bmp");
	 glBindTexture(GL_TEXTURE_2D, terrainTexture2);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	 waterTexture = loadSOIL("assets/water.bmp");
	 glBindTexture(GL_TEXTURE_2D, waterTexture);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	 waterTexture2 = loadSOIL("assets/water2.bmp");
	 glBindTexture(GL_TEXTURE_2D, waterTexture2);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	bottomTexture = loadSOIL("assets/water.bmp");
	glBindTexture(GL_TEXTURE_2D, bottomTexture);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	maskTexture = loadSOIL("assets/lake_mask.bmp");

	sunTexture = loadSOIL("assets/fiery.bmp");
	glBindTexture(GL_TEXTURE_2D, sunTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, 0x2803, GL_REPEAT);

	skyTexture = loadSOIL("assets/sky3.jpg");
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix) {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, W_WIDTH, W_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);

	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	// sky 
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	mat4 skydomeModelMatrix = glm::translate(mat4(1.0f), camera->position) * glm::scale(mat4(1.0f), vec3(30.0f));
	glUniform1f(normDirLocation, -1.0f); // invert normals
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &skydomeModelMatrix[0][0]); // add to vertex shader as M

	// --- NEW SKY TEXTURE SETUP ---
	glUniform1i(useTextureLocation, 3); // Set mode to 3 for Skydome logic in shader
	glActiveTexture(GL_TEXTURE7);       // Activate a new texture unit (7)
	glBindTexture(GL_TEXTURE_2D, skyTexture); // Bind your new sky texture

	// fragment shader: uniform sampler2D skyTex; // And get its location in C++:
	// GLuint skyTexLocation = glGetUniformLocation(shaderProgram, "skyTex");
	// Then set the texture unit:
	glUniform1i(glGetUniformLocation(shaderProgram, "skyTex"), 7);
	// -----------------------------

	sphere->bind();
	sphere->draw();
	//glEnable(GL_CULL_FACE);
	//glUniform1f(normDirLocation, 1.0f); // invert normals AGAIN
	//glUniform1i(useTextureLocation, 1);
	
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	// Upload light(s)
	uploadLight(*light); //??? giati 2 

	// Use material, not textures
	//uploadMaterial(turquoise);
	//glUniform1i(useTextureLocation, 0); // critical: no textures

	/*TERRAIN TEXTURE*/
	glUniform1i(useTextureLocation, 1); // use textures
	glUniform1f(normDirLocation, 1.0f);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, terrainTexture);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, terrainTexture2);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, waterTexture);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, waterTexture2);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, bottomTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, maskTexture);

	glUniform1i(glGetUniformLocation(shaderProgram, "terrainTex"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram, "terrainTex2"), 1);
	glUniform1i(glGetUniformLocation(shaderProgram, "waterTex"), 2);
	glUniform1i(glGetUniformLocation(shaderProgram, "waterTex2"), 3);
	glUniform1i(glGetUniformLocation(shaderProgram, "bottomTex"), 4);
	glUniform1i(glGetUniformLocation(shaderProgram, "maskTex"), 5);
	


	glUniform1f(glGetUniformLocation(shaderProgram, "time"), glfwGetTime());


	

	// Model matrix for terrain
	// scale *50

	float scaling_factor = SCALING_FACTOR;
	mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) 	* scale(mat4(), vec3(scaling_factor));

	// choose repeats_on_surface = number of tiles you want to see on the terrain
	float repeats_on_surface = 600.0f;

	// If you want 'repeats_on_surface' after the model scale is applied (i.e. world-space tiling):
	float uvTile = repeats_on_surface / scaling_factor;

	// If you wanted simple UV-space tiling (ignores model scale), use:
	//float uvTile = repeats_on_surface;

	// upload uv scale (same for U and V)
	glUniform2f(uvScaleLocation, uvTile, uvTile);

	// send scaling factor to shader
	glUniform1f(scaling_factor_location, scaling_factor);
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix[0][0]);

	terrain->bind();
	terrain->draw();

	/* <========  SUN  ========> */

	//light with the sunTexture
	glUniform1i(useTextureLocation, 2);
	glUniform1f(normDirLocation, -1.0f);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, sunTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "sunTex"), 6);
	// Model matrix for sun
	mat4 sunModel = translate(mat4(), light->lightPosition_worldspace) * scale(mat4(), vec3(0.8f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &sunModel[0][0]);
	sphere->bind();
	sphere->draw();
	/*light with simple matterial version
	// Save previous material state if needed (we'll set simple material)
	glUniform1i(useTextureLocation, 0);


	// <======== LIGHT SOURCE VISUALIZATION ========>
	//REVERSE THE NORMALS!!!
	glUniform1f(normDirLocation, -1.0f);
	// Light 1 sphere
	{
		// small emissive-looking material based on light color
		Material lm1{
			vec4(0.05f * light->Ld.r, 0.05f * light->Ld.g, 0.05f * light->Ld.b, 1.0f), // Ka
			light->Ld, // Kd
			vec4(1.0f, 1.0f, 1.0f, 1.0f), // Ks
			32.0f
		};
		uploadMaterial(lm1);

		mat4 lightModel = translate(mat4(), light->lightPosition_worldspace) * scale(mat4(), vec3(0.2f));
		glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &lightModel[0][0]);

		model2->bind(); // reuse the loaded sphere (earth.obj)
		model2->draw();
	}
	*/
	//// Light 2 sphere
	//{
	//	Material lm2{
	//		vec4(0.05f * light2->Ld.r, 0.05f * light2->Ld.g, 0.05f * light2->Ld.b, 1.0f), // Ka
	//		light2->Ld, // Kd
	//		vec4(1.0f, 1.0f, 1.0f, 1.0f), // Ks
	//		32.0f
	//	};
	//	uploadMaterial(lm2);

	//	mat4 lightModel2 = translate(mat4(), light2->lightPosition_worldspace) * scale(mat4(), vec3(0.2f));
	//	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &lightModel2[0][0]);

	//	model2->bind();
	//	model2->draw();
	//}
	// reset normals!!!!!!!!
	glUniform1f(normDirLocation, 1.0f);

}

void depth_pass(mat4 viewMatrix, mat4 projectionMatrix, GLuint depthFBO) {
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(depthProgram);

	mat4 view_projection = projectionMatrix * viewMatrix;
	glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &view_projection[0][0]);

	// Terrain model matrix
	//mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f));
	//glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	float scaling_factor = 10.0f;
	mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) * scale(mat4(), vec3(scaling_factor, scaling_factor, scaling_factor));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	terrain->bind();
	terrain->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// Task 2.3: visualize the depth_map on a sub-window at the top of the screen
void renderMiniMap() {
	// using the correct shaders to visualize the depth texture on the quad
	glUseProgram(miniMapProgram);

	//enabling the texture - follow the aforementioned pipeline
	glActiveTexture(GL_TEXTURE0); //gia allon shader apo prin to GL_TEXTURE0
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(quadTextureSamplerLocation, 0);
	// Drawing the quad
	quad->bind();
	quad->draw();
}



void mainLoop() {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	light->update();
	//light2->update();
	mat4 light_proj = light->projectionMatrix;
	mat4 light_view = light->viewMatrix;
	//mat4 light2_proj = light2->projectionMatrix; //hw2
	//mat4 light2_view = light2->viewMatrix; //hw2
	// Task 3.3
	// Create the depth buffer
	depth_pass(light_view, light_proj, depthFBO); // Call the depth pass once at the beginning
	//depth_pass(light2_view, light2_proj, depthFBO2); //hw2

	do {
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
			Light::chosen_light_id = 1;
		}
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
			Light::chosen_light_id = 2;
		}
		if (Light::chosen_light_id == 1) {
			light->update();
		}
		else if (Light::chosen_light_id == 2) {
			light2->update();
		}

		mat4 light_proj = light->projectionMatrix;
		mat4 light_view = light->viewMatrix;
		//mat4 light2_proj = light2->projectionMatrix; //hw2
		//mat4 light2_view = light2->viewMatrix; //hw2
		// Task 3.5
		// Create the depth buffer
		depth_pass(light_view, light_proj, depthFBO); //ama einai mesa θα ριξει τα fps,   ,light2_view, light2_proj
		//depth_pass(light2_view, light2_proj, depthFBO2); //hw2
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//αν σταθερη φωτεινη πηγη δεν εχει νοημα να το κανω καθε frame
		// κάθε δευτερόλεπτο

		// Getting camera information
		camera->update();
		mat4 projectionMatrix = camera->projectionMatrix;
		mat4 viewMatrix = camera->viewMatrix;



		//lighting_pass(viewMatrix, projectionMatrix);

		// Task 1.5
		// Rendering the scene from light's perspective when F1 is pressed

		if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
			lighting_pass(light_view, light_proj);
		}
		else {
			// Render the scene from camera's perspective
			lighting_pass(viewMatrix, projectionMatrix);
		}
		//*/

		// Task 2.2:
		renderMiniMap();


		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

}


void initialize() {
	// Initialize GLFW
	if (!glfwInit()) {
		throw runtime_error("Failed to initialize GLFW\n");
	}


	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	// Open a window and create its OpenGL context
	window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		throw runtime_error(string(string("Failed to open GLFW window.") +
			" If you have an Intel GPU, they are not 3.3 compatible." +
			"Try the 2.1 version.\n"));
	}
	glfwMakeContextCurrent(window);

	// Start GLEW extension handler
	glewExperimental = GL_TRUE;

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		glfwTerminate();
		throw runtime_error("Failed to initialize GLEW\n");
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Hide the mouse and enable unlimited movement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

	// Gray background color
	glClearColor(0.27f, 0.537f, 0.725f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// enable texturing and bind the depth texture
	glEnable(GL_TEXTURE_2D);

	// Log
	logGLParameters();

	// Create camera
	camera = new Camera(window);

	// Task 1.1 Creating a light source
	// Creating a custom light 
	light = new Light(window,
		vec4{ 0.894f, 0.949f, 0.949f, 1 },
		vec4{ 0.894f, 0.949f, 0.949f, 1 },
		vec4{ 0.894f, 0.949f, 0.949f, 1 },
		vec3{ 0, 20, -5 }
	);
	light2 = new Light(window,
		vec4{ 1, 1, 1, 1 },
		vec4{ 1, 1, 1, 1 },
		vec4{ 1, 1, 1, 1 },
		vec3{ -1, 5, -5 }
	);
}

int main(void) {
	try {
		initialize();
		
		createContext();
		mainLoop();
		free();
	}
	catch (exception& ex) {
		cout << ex.what() << endl;
		getchar();
		free();
		return -1;
	}

	return 0;
}