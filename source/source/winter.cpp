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
#include "common/cloud.h"
#include "common/forest.h"
#include <vector>



#define SCALING_FACTOR 200//60 //lab.cpp kai camera.cpp




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

#define SHADOW_WIDTH 4096//2048    8192
#define SHADOW_HEIGHT 4096//2048  8192




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
GLuint lightDirectionLocation;
// Global instance to hold your terrain data after loading

Drawable* quad;

// tree
Drawable* treeModel1; 
Drawable* bushModel;
GLuint treeDiffuseTex;
GLuint treeDiffuseTex2;
GLuint bushTexture1;
GLuint bushTexture2;
GLuint bushTexture3;
GLuint chrysTexture;
GLuint useInstancingLocation;


// locations for shaderProgram
GLuint scaling_factor_location;
GLuint viewMatrixLocation;
GLuint projectionMatrixLocation;
GLuint modelMatrixLocation;
GLuint KaLocation, KdLocation, KsLocation, NsLocation;
GLuint LaLocation, LdLocation, LsLocation;

GLuint lightPositionLocation;

GLuint lightPowerLocation;
GLuint diffuseColorSampler;
GLuint specularColorSampler;
GLuint useTextureLocation;
GLuint depthMapSampler;
GLuint lightVPLocation;



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
// In winter.cpp
GLuint trunkTexture ;
GLuint needleTexture ;
// locations for miniMapProgram
GLuint quadTextureSamplerLocation;

GLuint normDirLocation;

// clouds
CloudSystem* cloudSystem;

//forest
Forest* forest;

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
	glUniform3fv(lightDirectionLocation, 1,
		&light.direction[0]);
	/*glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);*/
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
	delete cloudSystem;
	// Delete Shader Programs
	glDeleteProgram(shaderProgram);
	glDeleteProgram(depthProgram);
	glDeleteProgram(miniMapProgram);
	delete forest;
	glfwTerminate();
}
void createContext() {
	useInstancingLocation = glGetUniformLocation(shaderProgram, "useInstancing");
	//glUniform1i(useInstancingLocation, 0);
	// Create and compile our GLSL program from the shader
	shaderProgram = loadShaders("ShadowMapping.vertexshader", "ShadowMapping.fragmentshader");

	// Task 3.1 
	// Create and load the shader program for the depth buffer construction
	// You need to load and use the Depth.vertexshader, Depth.fragmentshader
	depthProgram = loadShaders("Depth.vertexshader", "Depth.fragmentshader");


	// Task 2.1
	// Use the MiniMap.vertexshader, "MiniMap.fragmentshader"
	miniMapProgram = loadShaders("MiniMap.vertexshader", "MiniMap.fragmentshader");


	glUseProgram(shaderProgram); //??
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
	//lightPositionLocation2 = glGetUniformLocation(shaderProgram, "light2.lightPosition_worldspace");
	lightDirectionLocation =
		glGetUniformLocation(shaderProgram, "lightDirection_worldspace");
	//std::cout << "lightDirectionLocation: " << lightDirectionLocation << std::endl;
	
	diffuseColorSampler = glGetUniformLocation(shaderProgram, "diffuseColorSampler");
	specularColorSampler = glGetUniformLocation(shaderProgram, "specularColorSampler");
	scaling_factor_location= glGetUniformLocation(shaderProgram, "scaling_factor");
	uvScaleLocation = glGetUniformLocation(shaderProgram, "uvScale"); // <-- new

	
	//hw 4
	normDirLocation = glGetUniformLocation(shaderProgram, "normDir");
	// Task 1.4
	useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

	// locations for shadow rendering
	depthMapSampler = glGetUniformLocation(shaderProgram, "shadowMapSampler");
	lightVPLocation = glGetUniformLocation(shaderProgram, "lightVP");

	// --- depthProgram ---
	shadowViewProjectionLocation = glGetUniformLocation(depthProgram, "VP");
	shadowModelLocation = glGetUniformLocation(depthProgram, "M");
	//shadowViewProjectionLocation2 = glGetUniformLocation(depthProgram, "VP2"); //hw2
	// --- miniMapProgram ---
	quadTextureSamplerLocation = glGetUniformLocation(miniMapProgram, "textureSampler");

	//cloud
	//uvRotationLocation = glGetUniformLocation(shaderProgram, "uvRotationAngle");




	// Loading a model
	// The terrain object from Gaea is loaded as terrain
	std::string modelPath = "assets/Mesher_LOD3.obj";
	terrain = new Drawable(modelPath);

	// Load suzanne model with textures for shadow demonstration
	model1 = new Drawable("suzanne.obj");
	modelDiffuseTexture = loadSOIL("suzanne_diffuse.bmp");
	modelSpecularTexture = loadSOIL("suzanne_specular.bmp");

	// model2 (sphere) is used for light visualization, keep loading it
	sphere = new Drawable("earth.obj");

	// <=============== tree ========================>
	// 
	// Load tree models
	// After glUseProgram(shaderProgram);
	GLuint nodePositionsLocation = glGetUniformLocation(shaderProgram, "nodePositions");
	if (nodePositionsLocation != -1) {
		vec3 defaultNodes[4] = {
			vec3(0, 0, 0),
			vec3(0, 0, 0),
			vec3(0, 0, 0),
			vec3(0, 0, 0)
		};
		glUniform3fv(nodePositionsLocation, 4, &defaultNodes[0][0]);
	}
	//tre oj
	treeModel1 = new Drawable("assets/tree.obj");

	// 3 tree textures
	treeDiffuseTex2 = loadSOIL("assets/fir.jpg");
	glBindTexture(GL_TEXTURE_2D, treeDiffuseTex2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	treeDiffuseTex = loadSOIL("assets/tree2.jpg");
	glBindTexture(GL_TEXTURE_2D, treeDiffuseTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	

	chrysTexture = loadSOIL("assets/chrys.jpg"); //best?
	glBindTexture(GL_TEXTURE_2D, chrysTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// tree
	trunkTexture = loadSOIL("assets/bark.jpg");
	glBindTexture(GL_TEXTURE_2D, trunkTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//needleTexture = loadSOIL("assets/fir.jpg");
	needleTexture = loadSOIL("assets/tree2.jpg");
	glBindTexture(GL_TEXTURE_2D, needleTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	 
	// FOREST SYSTEM
	forest = new Forest(treeModel1, shaderProgram, 1); // 100 trees
	float scale = SCALING_FACTOR; // 200
	forest->setTerrainBounds(
		-scale / 2, scale / 2,  // X bounds
		-scale / 2, scale / 2,  // Z bounds
		0.0f, 50.0f,        // Y bounds
		2.0f              // scaling factor //? the heightmap is already scaled
	);
	forest->loadTerrainBinary("assets/heightmap/terrain_data.bin");
	// Generate tree positions
	forest->generate();

	//BUSH
	bushModel = new Drawable("assets/bush.obj");
	// 3 bush textures
	bushTexture1 = loadSOIL("assets/pixel_bush.png");
	glBindTexture(GL_TEXTURE_2D, bushTexture1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	bushTexture2 = loadSOIL("assets/bush2.png");
	glBindTexture(GL_TEXTURE_2D, bushTexture2);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	bushTexture3 = loadSOIL("assets/bush3.png");
	glBindTexture(GL_TEXTURE_2D, bushTexture3);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	

	/* CLOUD SYSTEM */
	// Initialize cloud system
	cloudSystem = new CloudSystem();
	cloudSystem->initialize(shaderProgram);

	// Add some clouds
	cloudSystem->addCloud(vec3(0, 20, -10), 5.0f);
	cloudSystem->addCloud(vec3(15, 32, 5), 6.0f);
	cloudSystem->addCloud(vec3(-10, 18, -20), 4.5f);
	cloudSystem->addCloud(vec3(20, 52, -15), 5.5f);
	cloudSystem->addCloud(vec3(-25, 90, 8), 3.0f);
	

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
	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, 0x2803, GL_REPEAT);*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	skyTexture = loadSOIL("assets/sky5.jpg"); //sky5.jpg
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		cout << "OpenGL error after getting uniform locations: " << err << endl;
	}

	// Check if critical uniforms were found:
	if (lightDirectionLocation == -1) cout << "WARNING: lightDirection_worldspace not found!" << endl;
	if (projectionMatrixLocation == -1) cout << "WARNING: P not found!" << endl;
	if (viewMatrixLocation == -1) cout << "WARNING: V not found!" << endl;
	if (modelMatrixLocation == -1) cout << "WARNING: M not found!" << endl;
	//everything orange fix
	//// CRITICAL: Disable instancing attributes for non-instanced rendering
	//glDisableVertexAttribArray(4);
	//glDisableVertexAttribArray(5);
	//glDisableVertexAttribArray(6);
	//glDisableVertexAttribArray(7);
	//glDisableVertexAttribArray(8);

	//// Set default values for instancing attributes (all zeros)
	//glVertexAttrib4f(4, 0.0f, 0.0f, 0.0f, 0.0f);
	//glVertexAttrib4f(5, 0.0f, 0.0f, 0.0f, 0.0f);
	//glVertexAttrib4f(6, 0.0f, 0.0f, 0.0f, 0.0f);
	//glVertexAttrib4f(7, 0.0f, 0.0f, 0.0f, 0.0f);
	//glVertexAttribI4i(8, 0, 0, 0, 0);

}

// Helper to reset common states to prevent leakage
void resetDefaultStates() {
	glUniform1i(useInstancingLocation, 0);
	glUniform2f(uvScaleLocation, 1.0f, 1.0f);
	glUniform1f(normDirLocation, 1.0f);

	// Explicitly disable instancing attributes just in case
	for (int i = 4; i <= 8; i++) glDisableVertexAttribArray(i);
}
void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix, int screen_width, int screen_height) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);

	// CRITICAL: Ensure instancing is OFF before doing anything else
	glUniform1i(useInstancingLocation, 0);

	// Now proceed with Sky Dome...
	glDisable(GL_CULL_FACE);
	// Initial Setup
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	// 1. SKY DOME (Unlit, no shadows)
	resetDefaultStates();
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	glUniform1i(useTextureLocation, 3); // Sky mode
	glUniform1f(normDirLocation, -1.0f);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "skyTex"), 7);

	mat4 skyM = translate(mat4(1.0f), camera->position) * scale(mat4(1.0f), vec3(30.0f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &skyM[0][0]);
	sphere->bind();
	sphere->draw();

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	// 2. LIGHTING GLOBALS
	uploadLight(*light);
	mat4 lightVP = light->lightVP();
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
	glUniform3fv(lightDirectionLocation, 1, &light->direction[0]);

	// Bind Shadow Map once to a high slot
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(depthMapSampler, 8);

	// 3. TERRAIN
	resetDefaultStates();
	glUniform1i(useTextureLocation, 1); // Terrain mode

	float repeats_on_surface = 600.0f;
	float uvTile = repeats_on_surface / SCALING_FACTOR;
	glUniform2f(uvScaleLocation, uvTile, uvTile);
	glUniform1f(scaling_factor_location, SCALING_FACTOR);

	// Bind all terrain textures
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

	mat4 terrainM = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) * scale(mat4(), vec3(SCALING_FACTOR));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &terrainM[0][0]);
	terrain->bind();
	terrain->draw();

	// 4. FOREST (Instanced) - FIXED VERSION
	resetDefaultStates();
	glUniform1i(useInstancingLocation, 1);
	glUniform1i(useTextureLocation, 5); // Tree mode
	glUniform2f(uvScaleLocation, 1.0f, 1.0f); // Reset UV scale for trees

	// CRITICAL FIX: Bind tree textures BEFORE drawing
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, trunkTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "trunkTex"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, needleTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "needleTex"), 1);

	// Bind additional needle textures if you have them
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, treeDiffuseTex2); // fir.jpg
	glUniform1i(glGetUniformLocation(shaderProgram, "needleTex2"), 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, chrysTexture); // chrys.jpg  
	glUniform1i(glGetUniformLocation(shaderProgram, "needleTex3"), 3);

	forest->draw();

	// CRITICAL: Properly disable instancing after forest
	glUniform1i(useInstancingLocation, 0);
	for (int i = 4; i <= 8; i++) {
		glDisableVertexAttribArray(i);
	}

	// 5. SUN (Emissive)
	resetDefaultStates();
	glUniform1i(useTextureLocation, 2); // Sun mode
	glUniform1f(normDirLocation, -1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sunTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "sunTex"), 0);

	vec3 sunPos = vec3(35.0f, 50.0f, 20.0f);
	mat4 sunM = translate(mat4(1.0f), sunPos) * scale(mat4(1.0f), vec3(0.9f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &sunM[0][0]);
	sphere->bind();
	sphere->draw();

	// 6. BUSH
	resetDefaultStates();
	glUniform1i(useTextureLocation, 6); // Bush mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bushTexture2);
	glUniform1i(diffuseColorSampler, 0);

	mat4 bushM = translate(mat4(1.0f), vec3(22.0f, 3.0f, 20.0f)) * scale(mat4(1.0f), vec3(0.015f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &bushM[0][0]);
	bushModel->bind();
	bushModel->draw();

	// 7. SUZANNE
	resetDefaultStates();
	glUniform1i(useTextureLocation, 1);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, modelSpecularTexture);
	glUniform1i(diffuseColorSampler, 0);
	glUniform1i(specularColorSampler, 1);

	mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &suzanneM[0][0]);
	model1->bind();
	model1->draw();
}
void lighting_pass3(mat4 viewMatrix, mat4 projectionMatrix, int screen_width, int screen_height) {
	// Initial Setup
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	// 1. SKY DOME (Unlit, no shadows)
	resetDefaultStates();
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	glUniform1i(useTextureLocation, 3); // Sky mode [cite: 110]
	glUniform1f(normDirLocation, -1.0f);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "skyTex"), 7);

	mat4 skyM = translate(mat4(1.0f), camera->position) * scale(mat4(1.0f), vec3(30.0f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &skyM[0][0]);
	sphere->bind();
	sphere->draw();

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	// 2. LIGHTING GLOBALS
	uploadLight(*light);
	mat4 lightVP = light->lightVP();
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
	glUniform3fv(lightDirectionLocation, 1, &light->direction[0]);

	// Bind Shadow Map once to a high slot
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(depthMapSampler, 8);

	// 3. TERRAIN
	resetDefaultStates();
	glUniform1i(useTextureLocation, 1); // Terrain mode [cite: 73]

	float repeats_on_surface = 600.0f;
	float uvTile = repeats_on_surface / SCALING_FACTOR;
	glUniform2f(uvScaleLocation, uvTile, uvTile);
	glUniform1f(scaling_factor_location, SCALING_FACTOR);

	// Bind all terrain textures [cite: 73, 74, 75, 76]
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, terrainTexture);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, terrainTexture2);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, waterTexture);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, waterTexture2);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, bottomTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, maskTexture);
	glUniform1f(glGetUniformLocation(shaderProgram, "time"), glfwGetTime());

	mat4 terrainM = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) * scale(mat4(), vec3(SCALING_FACTOR));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &terrainM[0][0]);
	terrain->bind();
	terrain->draw();

	// 4. FOREST (Instanced)
	resetDefaultStates();
	glUniform1i(useInstancingLocation, 1);
	glUniform1i(useTextureLocation, 5); // Tree mode [cite: 63]

	// Bind Tree Textures to slots 0 and 1 [cite: 63, 64]
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, trunkTexture);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, needleTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "trunkTex"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram, "needleTex"), 1);

	//forest->draw();

	// 5. SUN (Emissive)
	resetDefaultStates();
	glUniform1i(useTextureLocation, 2); // Sun mode [cite: 12]
	glUniform1f(normDirLocation, -1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sunTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "sunTex"), 0);

	vec3 sunPos = vec3(35.0f, 50.0f, 20.0f);
	mat4 sunM = translate(mat4(1.0f), sunPos) * scale(mat4(1.0f), vec3(0.9f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &sunM[0][0]);
	sphere->bind();
	sphere->draw();

	// 6. BUSH
	resetDefaultStates();
	glUniform1i(useTextureLocation, 6); // Bush mode [cite: 79]
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bushTexture2);
	glUniform1i(diffuseColorSampler, 0);

	mat4 bushM = translate(mat4(1.0f), vec3(22.0f, 3.0f, 20.0f)) * scale(mat4(1.0f), vec3(0.015f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &bushM[0][0]);
	bushModel->bind();
	bushModel->draw();

	// 7. SUZANNE
	resetDefaultStates();
	glUniform1i(useTextureLocation, 1); // Using mode 1 for generic texturing
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, modelSpecularTexture);
	glUniform1i(diffuseColorSampler, 0);
	glUniform1i(specularColorSampler, 1);

	mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &suzanneM[0][0]);
	model1->bind();
	model1->draw();
}

void lighting_pass2(mat4 viewMatrix, mat4 projectionMatrix, int screen_width, int screen_height) {
	// 1. Initial State Reset
	glUniform1i(useInstancingLocation, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

	// --- SKY DOME (Non-instanced) ---
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glUniform1i(useTextureLocation, 3); 
		mat4 skydomeModelMatrix = glm::translate(mat4(1.0f), camera->position) * glm::scale(mat4(1.0f), vec3(30.0f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &skydomeModelMatrix[0][0]);
	sphere->bind();
	sphere->draw();
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	// --- TERRAIN (Non-instanced) ---
	glUniform1i(useTextureLocation, 1); 
		uploadLight(*light);
	mat4 lightVP = light->lightVP();
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);

	// Bind terrain textures [cite: 38, 39]
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





	float scaling_factor = SCALING_FACTOR;
	mat4 terrainM = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) * scale(mat4(), vec3(scaling_factor));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &terrainM[0][0]);
	terrain->bind();
	terrain->draw();

	// --- FOREST (Instanced) ---
	glUniform1i(useInstancingLocation, 1); 
	//forest->draw();

	// --- CRITICAL RESET: Disable Instancing Attributes after Forest ---
	glUniform1i(useInstancingLocation, 0); 
		glDisableVertexAttribArray(4); // Matrix row 0 [cite: 2]
	glDisableVertexAttribArray(5); // Matrix row 1 [cite: 3]
	glDisableVertexAttribArray(6); // Matrix row 2 [cite: 3]
	glDisableVertexAttribArray(7); // Matrix row 3 [cite: 3]
	glDisableVertexAttribArray(8); // Texture Index [cite: 4]

	// --- SUN (Non-instanced) ---
	glUniform1i(useTextureLocation, 2); 
		vec3 sunPos = vec3(35.0f, 50.0f, 20.0f);
	mat4 sunModel = translate(mat4(1.0f), sunPos) * scale(mat4(1.0f), vec3(0.9f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &sunModel[0][0]);
	sphere->bind();
	sphere->draw();

	// --- SUZANNE (Non-instanced) ---
	glUniform1i(useTextureLocation, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);
	mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &suzanneM[0][0]);
	model1->bind();
	model1->draw();
}
void lighting_pass_old(mat4 viewMatrix, mat4 projectionMatrix, int screen_width, int screen_height) {
	glUniform1i(useInstancingLocation, 0); // disable instancing for terrain and sky


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glViewport(0, 0, W_WIDTH, W_HEIGHT);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);
	GLenum err = glGetError();
	/*if (err != GL_NO_ERROR) {
		std::cout << "OpenGL Error after useProgram: " << err << std::endl;
	}*/
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
	mat4 lightVP = light->lightVP();
	glUniformMatrix4fv(lightVPLocation, 1, GL_FALSE, &lightVP[0][0]);
	glUniform3fv(lightDirectionLocation, 1, &light->direction[0]);
	//std::cout << "Light direction: " << light->direction.x << ", "
	//	<< light->direction.y << ", " << light->direction.z << std::endl;
	//std::cout << "Light position: " << light->lightPosition_worldspace.x << ", "
	//	<< light->lightPosition_worldspace.y << ", "
	//	<< light->lightPosition_worldspace.z << std::endl;
	// Use material, not textures
	//uploadMaterial(turquoise);
	//glUniform1i(useTextureLocation, 0); // critical: no textures

	/*TERRAIN TEXTURE*/
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(depthMapSampler, 8);
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
	// Calculate sun position relative to camera
	// vec3 sunPos = camera->position - light->direction * 100.0f;

	// USE a fixed world position for the light marker
	vec3 sunPos = vec3(35.0f, 50.0f, 20.0f);

	// USE sunPos, not light->lightPosition_worldspace!
	mat4 sunModel = translate(mat4(1.0f), sunPos) * scale(mat4(1.0f), vec3(0.9f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &sunModel[0][0]);
	sphere->bind();
	sphere->draw();
	// Reset normals
	glUniform1f(normDirLocation, 1.0f);
	/*light with simple matterial version
	// Save previous material state if needed (we'll set simple material)
	glUniform1i(useTextureLocation, 0);


	/*=============================================================== LOAD TREE ===========================================================*/
	///   /\
	///  /  \
    ///   ||
	/*glUniform1i(useTextureLocation, 5);

	// Trunk → texture unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, trunkTexture);
	glUniform1i(glGetUniformLocation(shaderProgram, "trunkTex"), 0);

	// Needles → texture unit 1
	glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, needleTexture);
	glBindTexture(GL_TEXTURE_2D, chrysTexture);  //chrysantemum tezture looks good on tree!
	//glUniform1i(glGetUniformLocation(shaderProgram, "needleTex"), 1);
	glUniform1i(glGetUniformLocation(shaderProgram, "needleTex"), 1);

	// Optional: stronger tiling for tree
	glUniform2f(uvScaleLocation, 1.0f, 1.0f);

	// 3. Create the Model Matrix (Position the tree)
	// Place it at a specific coordinate, e.g., (10, 0, 10)
	mat4 treeM = translate(mat4(1.0f), vec3(20.0f, 3.0f, 20.0f)) * scale(mat4(1.0f), vec3(0.5f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &treeM[0][0]);

	// 4. Draw
	treeModel1->bind();
	treeModel1->draw();*/
	glUniform1i(useInstancingLocation, 1);  // Enable instancing
	forest->draw();
	glUniform1i(useInstancingLocation, 0);  // Disable 
	glDisableVertexAttribArray(4); // instanceMatrix_row0
	glDisableVertexAttribArray(5); // ...
	glDisableVertexAttribArray(6);
	glDisableVertexAttribArray(7);
	glDisableVertexAttribArray(8);


	// Reset UV scale for other objects
	glUniform2f(uvScaleLocation, 1.0f, 1.0f);

	/*==================== DRAW BUSH ====================*/
// 1. Set the texture (using bushTexture1 as an example)
	glUniform1i(useTextureLocation, 6); // bush texture mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bushTexture2);
	glUniform1i(diffuseColorSampler, 0);

	// 2. Position the bush next to the tree
	// The tree is at (20, 3, 20). We place the bush slightly to the side at (22, 3, 20).
	mat4 bushM = translate(mat4(1.0f), vec3(22.0f, 3.0f, 20.0f)) * scale(mat4(1.0f), vec3(0.015f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &bushM[0][0]);

	// 3. Draw
	bushModel->bind();
	bushModel->draw();

	/* <======== SUZANNE MODEL ========> */
	// Use textures for suzanne
	glUniform1i(useTextureLocation, 1);
	glUniform1f(normDirLocation, 1.0f);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);
	glUniform1i(diffuseColorSampler, 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, modelSpecularTexture);
	glUniform1i(specularColorSampler, 1);
	
	// Position suzanne in the scene
	mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &suzanneM[0][0]);
	
	model1->bind();
	model1->draw();
	
	glUniform1f(normDirLocation, 1.0f);

	
}

void depth_pass(mat4 viewMatrix, mat4 projectionMatrix, GLuint depthFBO) {
	glUniform1i(useInstancingLocation, 0);  // Disable instancing for depth pass

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(depthProgram);

	mat4 view_projection = projectionMatrix * viewMatrix;
	glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &view_projection[0][0]);

	// Terrain model matrix
	//mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f));
	//glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	float scaling_factor = SCALING_FACTOR;
	mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) * scale(mat4(), vec3(scaling_factor, scaling_factor, scaling_factor));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	terrain->bind();
	terrain->draw();

	// Use the same model matrix used in the lighting pass
	//mat4 treeM = translate(mat4(1.0f), vec3(10.0f, 0.0f, 10.0f)) * scale(mat4(1.0f), vec3(0.5f));
	/*mat4 treeM = translate(mat4(1.0f), vec3(20.0f, 3.0f, 20.0f))
		* scale(mat4(1.0f), vec3(0.5f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &treeM[0][0]); */

	//treeModel1->bind();
	//treeModel1->draw();

	// Draw forest in shadow pass
	glUniform1i(useInstancingLocation, 1);  // Enable instancing
	//forest->draw();
	glUniform1i(useInstancingLocation, 0);  // Disable 

	// Reset to standard texturing for Suzanne and others
	glUniform1i(useTextureLocation, 1);
	glUniform2f(uvScaleLocation, 1.0f, 1.0f);
	// Render suzanne in shadow pass
	mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &suzanneM[0][0]);
	
	model1->bind();
	model1->draw();

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
	float lastTime = glfwGetTime();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	light->update();
	
	mat4 light_proj = light->projectionMatrix;
	mat4 light_view = light->viewMatrix;
	//mat4 light2_proj = light2->projectionMatrix; //hw2
	//mat4 light2_view = light2->viewMatrix; //hw2
	// Task 3.3
	// Create the depth buffer
	depth_pass(light_view, light_proj, depthFBO); // Call the depth pass once at the beginning
	//depth_pass(light2_view, light2_proj, depthFBO2); //hw2
	
	// get screen size
	int fb_width, fb_height;
	glfwGetFramebufferSize(window, &fb_width, &fb_height);

	do {
		// Calculate delta time
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		/*if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
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
		}*/

		//mat4 light_proj = light->projectionMatrix;
		//mat4 light_view = light->viewMatrix;
		//mat4 light2_proj = light2->projectionMatrix; //hw2
		//mat4 light2_view = light2->viewMatrix; //hw2
		// Task 3.5
		// Create the depth buffer
		//depth_pass(light_view, light_proj, depthFBO); //ama einai mesa θα ριξει τα fps,   ,light2_view, light2_proj
		//depth_pass(light2_view, light2_proj, depthFBO2); //hw2
		
		light->update();
		cloudSystem->update(deltaTime);

		// Getting camera information
		camera->update();
		mat4 projectionMatrix = camera->projectionMatrix;
		mat4 viewMatrix = camera->viewMatrix;

		// frustum fit
		light->fitToCameraFrustum(viewMatrix, projectionMatrix);

		// Re-fetch updated light matrices
		mat4 light_proj = light->projectionMatrix;
		mat4 light_view = light->viewMatrix;

		// Now render shadow map
		depth_pass(light_view, light_proj, depthFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//αν σταθερη φωτεινη πηγη δεν εχει νοημα να το κανω καθε frame
		// κάθε δευτερόλεπτο

		// Task 1.5
		// Rendering the scene from light's perspective when F1 is pressed

		if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
			//lighting_pass(light_view, light_proj);
			lighting_pass(light_view, light_proj, fb_width, fb_height);
		}
		else {
			// Render the scene from camera's perspective
			//lighting_pass(viewMatrix, projectionMatrix);
			lighting_pass(viewMatrix, projectionMatrix, fb_width, fb_height);
		}

		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
			// Spawn cloud at random position
			vec3 pos = vec3(
				-60.0f + (rand() % 120),  // X: -30 to 60
				15.0f + (rand() % 45),    // Y: 15 to 60
				-60.0f + (rand() % 120)    // Z: -60 to 60
			);
			float size = 4.0f + (rand() % 5); // Size: 4 to 9
			cloudSystem->addCloud(pos, size);
		}

		//*/
		// Render clouds
		glUseProgram(shaderProgram);
		glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
		glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
		cloudSystem->render(viewMatrix, projectionMatrix);
		// Task 2.2:
		//renderMiniMap();


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
	//window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
	
	// 1. Get the primary monitor handle
	GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

	// 2. Get the video mode of the primary monitor to use its resolution
	const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

	// 3. Create the window, passing the monitor handle and using its resolution
	// The W_WIDTH and W_HEIGHT are replaced by the monitor's resolution for clarity,
	// but you can still use your variables if you prefer.
	// The last two NULL arguments are for the monitor and share context respectively.
	window = glfwCreateWindow(mode->width, mode->height, TITLE, primary_monitor, NULL);

	if (window == NULL) {
		glfwTerminate();
		throw runtime_error(string(string("Failed to open GLFW window.") +
			" If you have an Intel GPU, they are not 3.3 compatible." +
			"Try the 2.1 version.\n"));
	}
	glfwMakeContextCurrent(window);
	//!!
	// local variables to hold the current window size
	int current_width, current_height;
	glfwGetFramebufferSize(window, &current_width, &current_height);

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
	glEnable(GL_DEPTH_CLAMP);
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
		vec4{ 0.7f, 0.75f, 0.85f, 1 },   // La - cool ambient (blueish)
		vec4{ 0.85f, 0.9f, 0.95f, 1 },   // Ld - cool diffuse (slightly blue-white)
		vec4{ 0.9f, 0.95f, 1.0f, 1 },    // Ls - cool specular (white-blue)
		normalize(vec3(-0.3f, -1.0f, -0.2f))   // SUN DIRECTION
	);
	/*light2 = new Light(window,
		vec4{ 1, 1, 1, 1 },
		vec4{ 1, 1, 1, 1 },
		vec4{ 1, 1, 1, 1 },
		vec3{ -1, 5, -5 }
	);*/
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