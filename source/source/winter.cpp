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

#define FULL_SCREEN 0
#define W_WIDTH  1800
#define W_HEIGHT  900
#define TITLE "Winter"

#define SHADOW_WIDTH 4096//2048    8192
#define SHADOW_HEIGHT 4096//2048  8192

// Global Variables
GLFWwindow* window;
Camera* camera;
Light* light;
//Light* light2;
Drawable* model1;
Drawable* sphere;
Drawable* terrain;
Drawable* plane;
GLuint modelDiffuseTexture, modelSpecularTexture;
GLuint depthFBO, depthTexture;
GLuint depthFBO2, depthTexture2;
// Global instance to hold your terrain data after loading

Drawable* quad;

// tree
Drawable* treeModel1; 
Drawable* bushModel;
Drawable* deerModel;

GLuint lightPowerLocation;

// locations for programs.depth
GLuint shadowViewProjectionLocation;
GLuint shadowModelLocation;
//GLuint shadowViewProjectionLocation2;

//terrain
GLuint terrainTexture ;
GLuint terrainTexture2 ;
GLuint waterTexture ;
GLuint waterTexture2 ;
GLuint bottomTexture ;
GLuint maskTexture;
GLuint sunTexture;
GLuint skyTexture;
// tree
GLuint trunkTexture ;
GLuint needleTexture ;
GLuint treeDiffuseTex2;
GLuint chrysTexture;
//bush
GLuint bushTexture1;
GLuint bushTexture2;
GLuint bushTexture3;
//deer
GLuint deerTexture;
// locations for miniMapProgram
//GLuint quadTextureSamplerLocation;


// clouds
CloudSystem* cloudSystem;

//forest
Forest* forest;

// Creating a structure to store the material parameters of an object
struct Material
{
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	float Ns;
};

struct TexLocations {
	// terrain
	GLuint terrainTex;
	GLuint terrainTex2;
	GLuint waterTex;
	GLuint waterTex2;
	GLuint bottomTex;
	GLuint maskTex;

	// vegetation
	GLuint trunkTex;
	GLuint needleTex;
	GLuint needleTex2;
	GLuint needleTex3;

	// sky & sun
	GLuint skyTex;
	GLuint sunTex;
};

struct Programs {
	GLuint lighting = 0;
	GLuint depth = 0;
	GLuint miniMap = 0;
};

struct Uniforms {
	// matrices
	GLuint P = 0;
	GLuint V = 0;
	GLuint M = 0;

	// material
	GLuint Ka = 0;
	GLuint Kd = 0;
	GLuint Ks = 0;
	GLuint Ns = 0;

	// light
	GLuint La = 0;
	GLuint Ld = 0;
	GLuint Ls = 0;
	GLuint lightDir = 0;
	GLuint lightPos = 0;

	// rendering control
	GLuint useTexture = 0;
	GLuint useInstancing = 0;
	GLuint uvScale = 0;
	GLuint normDir = 0;
	GLuint scalingFactor = 0;

	// shadow
	GLuint depthMap = 0;
	GLuint lightVP = 0;

	// samplers
	GLuint diffuseSampler = 0;
	GLuint specularSampler = 0;
};

Programs programs;
Uniforms u;
TexLocations t;


GLuint loadTextureRepeat(const std::string& path) {
	GLuint tex = loadSOIL(path.c_str());
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return tex;
}

// NOTE: Since the Light and Material struct are used in the shader programs as well 
//		 it is recommended to create a function that will update all the parameters 
//       of an object.  // Creating a function to upload (make uniform) the light parameters to the shader program

void uploadLight(const Light& light) {
	glUniform4f(u.La, light.La.r, light.La.g, light.La.b, light.La.a);
	glUniform4f(u.Ld, light.Ld.r, light.Ld.g, light.Ld.b, light.Ld.a);
	glUniform4f(u.Ls, light.Ls.r, light.Ls.g, light.Ls.b, light.Ls.a);
	glUniform3fv(u.lightDir, 1,
		&light.direction[0]);
	/*glUniform3f(lightPositionLocation, light.lightPosition_worldspace.x,
		light.lightPosition_worldspace.y, light.lightPosition_worldspace.z);*/
}

// Creating a function to upload the material parameters of a model to the shader program
void uploadMaterial(const Material& mtl) {
	glUniform4f(u.Ka, mtl.Ka.r, mtl.Ka.g, mtl.Ka.b, mtl.Ka.a);
	glUniform4f(u.Kd, mtl.Kd.r, mtl.Kd.g, mtl.Kd.b, mtl.Kd.a);
	glUniform4f(u.Ks, mtl.Ks.r, mtl.Ks.g, mtl.Ks.b, mtl.Ks.a);
	glUniform1f(u.Ns, mtl.Ns);
}

void setShaderMode(int mode, bool instanced = false) {
	glUniform1i(u.useTexture, mode);
	glUniform1i(u.useInstancing, instanced ? 1 : 0);
	if (!instanced) {
		// Automatically clean up attributes
		for (int i = 4; i <= 8; i++) glDisableVertexAttribArray(i);
	}
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
	glDeleteProgram(programs.lighting);
	glDeleteProgram(programs.depth);
	delete forest;
	glfwTerminate();
}

void createContext() {
	programs.lighting = loadShaders("ShadowMapping.vertexshader", "ShadowMapping.fragmentshader");
	programs.depth = loadShaders("Depth.vertexshader", "Depth.fragmentshader");
	programs.miniMap = loadShaders("MiniMap.vertexshader", "MiniMap.fragmentshader");
	glUseProgram(programs.lighting);


	// THIS FIXED THE GIANT TREE ISSUE
	/* =====================================================================================
	====================================VERY IMPORTANT ====================================
	=====================================================================================*/
	u.useInstancing = glGetUniformLocation(programs.lighting, "useInstancing");
	glUniform1i(u.useInstancing, 0);
	// BEGIN WITH INSTANCING 0

	// NOTE: Don't forget to delete the shader programs on the free() function

	// Get pointers to uniforms
	// --- programs.lighting ---
	u.P = glGetUniformLocation(programs.lighting, "P");
	u.V = glGetUniformLocation(programs.lighting, "V");
	u.M = glGetUniformLocation(programs.lighting, "M");
	// for phong lighting
	u.Ka = glGetUniformLocation(programs.lighting, "mtl.Ka");
	u.Kd = glGetUniformLocation(programs.lighting, "mtl.Kd");
	u.Ks = glGetUniformLocation(programs.lighting, "mtl.Ks");
	u.Ns = glGetUniformLocation(programs.lighting, "mtl.Ns");

	u.La = glGetUniformLocation(programs.lighting, "light.La");
	u.Ld = glGetUniformLocation(programs.lighting, "light.Ld");
	u.Ls = glGetUniformLocation(programs.lighting, "light.Ls");

	u.lightPos = glGetUniformLocation(programs.lighting, "light.lightPosition_worldspace");//lightPositionLocation2 = glGetUniformLocation(programs.lighting, "light2.lightPosition_worldspace");
	u.lightDir = glGetUniformLocation(programs.lighting, "lightDirection_worldspace");
	//std::cout << "u.lightDir: " << u.lightDir << std::endl;
	
	u.diffuseSampler = glGetUniformLocation(programs.lighting, "u.diffuseSampler");
	u.specularSampler = glGetUniformLocation(programs.lighting, "u.specularSampler");
	u.scalingFactor = glGetUniformLocation(programs.lighting, "scaling_factor");
	u.uvScale = glGetUniformLocation(programs.lighting, "uvScale");
	
	u.normDir = glGetUniformLocation(programs.lighting, "normDir");
	u.useTexture = glGetUniformLocation(programs.lighting, "useTexture");

	// locations for shadow rendering
	u.depthMap = glGetUniformLocation(programs.lighting, "shadowMapSampler");
	u.lightVP = glGetUniformLocation(programs.lighting, "lightVP");

	// --- programs.depth ---
	shadowViewProjectionLocation = glGetUniformLocation(programs.depth, "VP");
	shadowModelLocation = glGetUniformLocation(programs.depth, "M");
	//shadowViewProjectionLocation2 = glGetUniformLocation(programs.depth, "VP2"); //hw2

	//cloud
	//uvRotationLocation = glGetUniformLocation(programs.lighting, "uvRotationAngle");

	// Loading a model
	// The terrain object from Gaea is loaded as terrain
	std::string modelPath = "assets/Mesher_LOD3.obj";
	terrain = new Drawable(modelPath);

	//// Load suzanne model with textures for shadow demonstration
	//model1 = new Drawable("suzanne.obj");
	//modelDiffuseTexture = loadSOIL("suzanne_diffuse.bmp");
	//modelSpecularTexture = loadSOIL("suzanne_specular.bmp");

	// Load deer model
	deerModel = new Drawable("assets/deer.obj");
	// deer texture
	deerTexture = loadSOIL("assets/deer_colored.png");

	// model2 (sphere) is used for light visualization, keep loading it
	sphere = new Drawable("earth.obj");

	// <=============== tree ========================>
	// 
	// Load tree models
	GLuint nodePositions = glGetUniformLocation(programs.lighting, "nodePositions");
	if (nodePositions != -1) {
		vec3 defaultNodes[4] = {
			vec3(0, 0, 0),
			vec3(0, 0, 0),
			vec3(0, 0, 0),
			vec3(0, 0, 0)
		};
		glUniform3fv(nodePositions, 4, &defaultNodes[0][0]);
	}
	//tre oj
	treeModel1 = new Drawable("assets/tree.obj");

	// 3 tree textures
	treeDiffuseTex2 = loadTextureRepeat("assets/fir.jpg");

	chrysTexture = loadTextureRepeat("assets/chrys.jpg"); //best?

	trunkTexture = loadTextureRepeat("assets/bark.jpg");

	needleTexture = loadTextureRepeat("assets/tree2.jpg");

	// FOREST SYSTEM
	forest = new Forest(treeModel1, programs.lighting, 100); // 100 trees
	float scale = SCALING_FACTOR; // 200
	forest->setTerrainBounds(
		-scale / 2, scale / 2,  // X bounds
		-scale / 2, scale / 2,  // Z bounds
		0.0f, 50.0f,        // Y bounds
		1.0f              // scaling factor //? the heightmap is already scaled
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
	cloudSystem->initialize(programs.lighting);

	// Add some clouds
	cloudSystem->addCloud(vec3(0, 20, -10), 5.0f);
	cloudSystem->addCloud(vec3(15, 32, 5), 6.0f);
	cloudSystem->addCloud(vec3(-10, 18, -20), 4.5f);
	cloudSystem->addCloud(vec3(20, 52, -15), 5.5f);
	cloudSystem->addCloud(vec3(-25, 90, 8), 3.0f);
	
	createDepthFBOAndTexture(depthFBO, depthTexture);

	// Homework 2: create second depth FBO and texture
	/*createDepthFBOAndTexture(depthFBO2, depthTexture2);*/

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

	//cache texture locations
	glUseProgram(programs.lighting);

	// terrain
	t.terrainTex = glGetUniformLocation(programs.lighting, "terrainTex");
	t.terrainTex2 = glGetUniformLocation(programs.lighting, "terrainTex2");
	t.waterTex = glGetUniformLocation(programs.lighting, "waterTex");
	t.waterTex2 = glGetUniformLocation(programs.lighting, "waterTex2");
	t.bottomTex = glGetUniformLocation(programs.lighting, "bottomTex");
	t.maskTex = glGetUniformLocation(programs.lighting, "maskTex");

	// vegetation
	t.trunkTex = glGetUniformLocation(programs.lighting, "trunkTex");
	t.needleTex = glGetUniformLocation(programs.lighting, "needleTex");
	t.needleTex2 = glGetUniformLocation(programs.lighting, "needleTex2");
	t.needleTex3 = glGetUniformLocation(programs.lighting, "needleTex3");

	// sky / sun
	t.skyTex = glGetUniformLocation(programs.lighting, "skyTex");
	t.sunTex = glGetUniformLocation(programs.lighting, "sunTex");

#define CHECK_TEX(x) if (x == -1) std::cout << "Missing sampler: " #x "\n";
	CHECK_TEX(t.terrainTex);
	CHECK_TEX(t.skyTex);
	CHECK_TEX(t.sunTex);

	
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		cout << "OpenGL error after getting uniform locations: " << err << endl;
	}

	// Check if critical uniforms were found:
	if (u.lightDir == -1) cout << "WARNING: lightDirection_worldspace not found!" << endl;
	if (u.P == -1) cout << "WARNING: P not found!" << endl;
	if (u.V == -1) cout << "WARNING: V not found!" << endl;
	if (u.M == -1) cout << "WARNING: M not found!" << endl;

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
	glUniform1i(u.useInstancing, 0);
	glUniform2f(u.uvScale, 1.0f, 1.0f);
	glUniform1f(u.normDir, 1.0f);

	// Explicitly disable instancing attributes just in case
	for (int i = 4; i <= 8; i++) glDisableVertexAttribArray(i);
}

void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix, int screen_width, int screen_height) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programs.lighting);

	// CRITICAL: Ensure instancing is OFF before doing anything else
	glUniform1i(u.useInstancing, 0);

	// Now proceed with Sky Dome...
	glDisable(GL_CULL_FACE);
	// Initial Setup
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programs.lighting);
	glUniformMatrix4fv(u.V, 1, GL_FALSE, &viewMatrix[0][0]);
	glUniformMatrix4fv(u.P, 1, GL_FALSE, &projectionMatrix[0][0]);

	// 1. SKY DOME (Unlit, no shadows)
	//sphere->bind();
	resetDefaultStates();
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	glUniform1i(u.useTexture, 3); // Sky mode
	glUniform1f(u.normDir, -1.0f);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glUniform1i(t.skyTex, 7);
	glUniform1i(u.useInstancing, 0); //?
	mat4 skyM = translate(mat4(1.0f), camera->position) * scale(mat4(1.0f), vec3(30.0f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &skyM[0][0]);
	sphere->bind();
	sphere->draw();

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	// 2. LIGHTING GLOBALS
	glUniform1i(u.useInstancing, 0);
	uploadLight(*light);
	mat4 lightVP = light->lightVP();
	glUniformMatrix4fv(u.lightVP, 1, GL_FALSE, &lightVP[0][0]);
	glUniform3fv(u.lightDir, 1, &light->direction[0]);

	// Bind Shadow Map once to a high slot
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glUniform1i(u.depthMap, 8);

	// 3. TERRAIN
	glUniform1i(u.useInstancing, 0);
	resetDefaultStates();
	glUniform1i(u.useTexture, 1); // Terrain mode

	float repeats_on_surface = 600.0f;
	float uvTile = repeats_on_surface / SCALING_FACTOR;
	glUniform2f(u.uvScale, uvTile, uvTile);
	glUniform1f(u.scalingFactor, SCALING_FACTOR);

	// Bind all terrain textures
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, terrainTexture);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, terrainTexture2);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, waterTexture);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, waterTexture2);
	glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, bottomTexture);
	glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, maskTexture);
	glUniform1i(t.terrainTex, 0);
	glUniform1i(t.terrainTex2, 1);
	glUniform1i(t.waterTex, 2);
	glUniform1i(t.waterTex2, 3);
	glUniform1i(t.bottomTex, 4);
	glUniform1i(t.maskTex, 5);
	glUniform1f(glGetUniformLocation(programs.lighting, "time"), glfwGetTime());

	mat4 terrainM = translate(mat4(), vec3(0.0f, 0.5f, 0.0f)) * scale(mat4(), vec3(SCALING_FACTOR));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &terrainM[0][0]);
	terrain->bind();
	terrain->draw();

	// 4. FOREST (Instanced) - FIXED VERSION
	resetDefaultStates();
	glUniform1i(u.useInstancing, 1);
	glUniform1i(u.useTexture, 5); // Tree mode
	glUniform2f(u.uvScale, 1.0f, 1.0f); // Reset UV scale for trees

	// CRITICAL FIX: Bind tree textures BEFORE drawing
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, trunkTexture);
	glUniform1i(t.trunkTex, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, needleTexture);
	glUniform1i(t.needleTex, 1);

	// Bind additional needle textures if you have them
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, treeDiffuseTex2); // fir.jpg
	glUniform1i(t.needleTex2, 2);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, chrysTexture); // chrys.jpg  
	glUniform1i(t.needleTex3, 3);

	forest->draw();

	// CRITICAL: Properly disable instancing after forest
	glUniform1i(u.useInstancing, 0);
	for (int i = 4; i <= 8; i++) {
		glDisableVertexAttribArray(i);
	}

	// 5. SUN (Emissive)
	resetDefaultStates();
	glUniform1i(u.useTexture, 2); // Sun mode
	glUniform1f(u.normDir, -1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sunTexture);
	glUniform1i(glGetUniformLocation(programs.lighting, "sunTex"), 0);

	// Use the light's actual position (updated by light->update())
	vec3 sunPos = light->sun_pos;
	mat4 sunM = translate(mat4(1.0f), sunPos) * scale(mat4(1.0f), vec3(2.0f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &sunM[0][0]);
	sphere->bind();
	sphere->draw();

	// 6. BUSH
	resetDefaultStates();
	glUniform1i(u.useTexture, 6); // Bush mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bushTexture2);
	glUniform1i(u.diffuseSampler, 0);

	mat4 bushM = translate(mat4(1.0f), vec3(22.0f, 4.0f, 20.0f)) * scale(mat4(1.0f), vec3(0.03f, 0.02f, 0.03f)); //anisotropic scaling 
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &bushM[0][0]);
	bushModel->bind();
	bushModel->draw();

	//// 7. SUZANNE
	//resetDefaultStates();
	//glUniform1i(u.useTexture, 1);
	//glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, modelDiffuseTexture);
	//glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, modelSpecularTexture);
	//glUniform1i(u.diffuseSampler, 0);
	//glUniform1i(u.specularSampler, 1);

	//mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	//glUniformMatrix4fv(u.M, 1, GL_FALSE, &suzanneM[0][0]);
	//model1->bind();
	//model1->draw();

	//8. DEER 
	//LOAD DEER OBJECT 
	resetDefaultStates();
	glUniform1i(u.useTexture, 6); // Bush & Deer mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, deerTexture);
	glUniform1i(u.diffuseSampler, 0);

	mat4 deerM = translate(mat4(1.0f), vec3(22.0f, 5.0f, 22.0f)) * scale(mat4(1.0f), vec3(1.0f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &deerM[0][0]);
	deerModel->bind();
	deerModel->draw();
}

void depth_pass(mat4 viewMatrix, mat4 projectionMatrix, GLuint depthFBO) {
	glUniform1i(u.useInstancing, 0);  // Disable instancing for depth pass

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(programs.depth);

	mat4 view_projection = projectionMatrix * viewMatrix;
	glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &view_projection[0][0]);

	// Terrain model matrix
	//mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f));
	//glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	//float scaling_factor = SCALING_FACTOR;
	//mat4 modelMatrix = translate(mat4(), vec3(0.0f, -1.0f, -5.0f)) * scale(mat4(), vec3(scaling_factor, scaling_factor, scaling_factor));
	//glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &modelMatrix[0][0]);
	//terrain->bind();
	//terrain->draw();

	// Use the same model matrix used in the lighting pass
	//mat4 treeM = translate(mat4(1.0f), vec3(10.0f, 0.0f, 10.0f)) * scale(mat4(1.0f), vec3(0.5f));
	/*mat4 treeM = translate(mat4(1.0f), vec3(20.0f, 3.0f, 20.0f))
		* scale(mat4(1.0f), vec3(0.5f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &treeM[0][0]); */

	//treeModel1->bind();
	//treeModel1->draw();

	// Draw forest in shadow pass
	glUniform1i(u.useInstancing, 1);  // Enable instancing
	forest->draw();
	glUniform1i(u.useInstancing, 0);  // Disable 

	// Reset to standard texturing for Suzanne and others
	glUniform1i(u.useTexture, 1);
	glUniform2f(u.uvScale, 1.0f, 1.0f);
	//// Render suzanne in shadow pass
	//mat4 suzanneM = translate(mat4(1.0f), vec3(-15.0f, 20.0f, -10.0f)) * scale(mat4(1.0f), vec3(1.5f));
	//glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &suzanneM[0][0]);
	//
	//model1->bind();
	//model1->draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
		glUseProgram(programs.lighting);
		glUniformMatrix4fv(u.V, 1, GL_FALSE, &viewMatrix[0][0]);
		glUniformMatrix4fv(u.P, 1, GL_FALSE, &projectionMatrix[0][0]);
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
	
	//// 1. Get the primary monitor handle
	//GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

	//// 2. Get the video mode of the primary monitor to use its resolution
	//const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

	//// 3. Create the window, passing the monitor handle and using its resolution
	//// The W_WIDTH and W_HEIGHT are replaced by the monitor's resolution for clarity,
	//// but you can still use your variables if you prefer.
	//// The last two NULL arguments are for the monitor and share context respectively.
	//window = glfwCreateWindow(mode->width, mode->height, TITLE, primary_monitor, NULL);
	// Open a window and create its OpenGL context
#if FULL_SCREEN == 1
	// Fullscreen mode
	GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
	window = glfwCreateWindow(mode->width, mode->height, TITLE, primary_monitor, NULL);
#else
	// Windowed mode
	window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
#endif
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
		normalize(vec3(-0.3f, -1.0f, -0.2f)),   // SUN DIRECTION
		150.0f  // radius
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

// Create two sample materials
//const Material polishedSilver
//{
//	vec4{0.23125, 0.23125, 0.23125, 1},
//	vec4{0.2775, 0.2775, 0.2775, 1},
//	vec4{0.773911, 0.773911, 0.773911, 1},
//	89.6f
//};
//
//const Material turquoise
//{
//	vec4{ 0.1, 0.18725, 0.1745, 0.8 },
//	vec4{ 0.396, 0.74151, 0.69102, 0.8 },
//	vec4{ 0.297254, 0.30829, 0.306678, 0.8 },
//	12.8f
//};
