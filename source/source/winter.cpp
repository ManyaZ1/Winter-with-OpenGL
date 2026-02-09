// Include C++ headers
#include <iostream>
#include <string>
// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
#include <fstream>
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
#include "common/bush.h"
#include "common/snow.h"
#include <vector>

#define AM_IMPLEMENTATION  // This "activates" the audio code here
#include "../common/AudioManager.h"
#define FULL_SCREEN 0
#define W_WIDTH  1280
#define W_HEIGHT  720
#define TITLE "Winter"
AudioManager audio;

#define SCALING_FACTOR 200//60 //lab.cpp kai camera.cpp

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();
float sampleHeightAt(
	float x, float z,
	const std::vector<float>& heightData,
	int gridResolution,
	float minX, float maxX,
	float minZ, float maxZ);
std::vector<float> getHeightDataOnly(const std::string& filePath);



#define SHADOW_WIDTH  16384 //8192// 4096//2048    8192
#define SHADOW_HEIGHT  16384//8192//4096//2048  8192

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
//GLuint depthFBO2, depthTexture2;
// Global instance to hold your terrain data after loading

Drawable* quad;

// tree
Drawable* treeModel1; 
Drawable* bushModel;
Drawable* appleTreeModel;
Drawable* pineTreeModel;
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
//deer animals
GLuint deerTexture;
GLuint bearTexture;
GLuint polarbearTexture;
GLuint appleTexture;
GLuint wolfTexture;
//snow
GLuint snowFlakeTexture;
GLuint snowTexture;
GLuint snowDetailTexture;
// locations for miniMapProgram
//GLuint quadTextureSamplerLocation;


// clouds
CloudSystem* cloudSystem;

//forest
//Forest* forest;
Forest* appleForest;
Forest* pineForest;
BushField* bushes;


//animals
Drawable* deerModel;
Drawable* bearModel;
Drawable* wolfModel;
float deerX; float deerZ; float deerY;
float bearX; float bearZ; float bearY;
float polarbearX; float polarbearZ; float polarbearY;
float wolfX; float wolfZ; float wolfY;

// ============================================================================================ //
							      //       WEATHER GLOBALS       //							                                          
// =========================================================================================== //
//SNOW globals
GLuint snowAccumFBO, snowAccumTexture;
bool snowMapGenerated = false;
//snow and fog
SnowSystem* snowSystem;
bool snowingEnabled = false;
float snowAccumulationTime = 0.0f;
float fogAccumulationTime = 0.0f;
float snowLevel = 0.0f;
float fogDensity = 0.0f;


// Creating a structure to store the material parameters of an object
struct Material
{
	vec4 Ka;
	vec4 Kd;
	vec4 Ks;
	float Ns;
};
//wind struct
struct WindState {
	bool active = false;
	int strength = 0;
	float xBias = 0.0f;
	float zBias = 0.0f;
};
void updateWind(GLFWwindow* window,WindState& wind,bool& vKeyPressed,float biasStep);

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

	GLuint bushTex1;
	GLuint bushTex2;
	GLuint bushTex3;

	GLuint appleTex;

	// sky & sun
	GLuint skyTex;
	GLuint sunTex;
	
	GLuint snowTex;
	GLuint  snowDetailTex;
};

struct Programs {
	GLuint lighting = 0;
	GLuint depth = 0;
	GLuint snow = 0;
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

	// wind
	GLuint wind = 0; //location of shadowmapping vertex shader for wind
	GLuint time = 0;
	GLuint winddepth = 0; // location of depth vertex shader for wind
	GLuint timedepth = 0;
	GLuint snowtime =0;
	GLuint snowWind;
	GLuint xbias = 0; // x_bias for wind direction
	GLuint zbias = 0; // z_bias for wind direction
	GLuint xbiasdepth = 0; // x_bias for depth shader
	GLuint zbiasdepth = 0; // z_bias for depth shader
	GLuint xbiassnow = 0; // x_bias for snow shader
	GLuint zbiassnow = 0; // z_bias for snow shader
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

	//snow
	GLuint snowAccumMap = 0;
	GLuint skyVP = 0;
	GLuint snowAmount = 0;

	// fog
	GLuint fogDensity = 0;
	GLuint fogColor = 0;
};

Programs programs;
Uniforms u;
TexLocations t;
std::vector<float> heightData;

GLuint loadTextureRepeat(const std::string& path) {
	//replace repeat lines, load repeat texture to shader with one line
	GLuint tex = loadSOIL(path.c_str());
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return tex;
}
//replaces these blocks as well
/*waterTexture = loadSOIL("assets/water.bmp");
glBindTexture(GL_TEXTURE_2D, waterTexture);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); */


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
	delete appleForest;
	delete pineForest;
	delete bushes;
	delete snowSystem;
	// Clean up snow resources
	glDeleteFramebuffers(1, &snowAccumFBO);
	glDeleteTextures(1, &snowAccumTexture);
	glfwTerminate();
}

void createContext() {
	programs.lighting = loadShaders("ShadowMapping.vertexshader", "ShadowMapping.fragmentshader");
	programs.depth = loadShaders("Depth.vertexshader", "Depth.fragmentshader");
	//programs.miniMap = loadShaders("MiniMap.vertexshader", "MiniMap.fragmentshader");
	programs.snow = loadShaders("Snow.vertexshader", "Snow.fragmentshader");
	if (programs.snow == 0) {
		cout << "ERROR: Snow shader failed to compile!" << endl;
	}
	else {
		cout << "Snow shader compiled successfully: " << programs.snow << endl;
	}
	
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
	// wind
	u.wind = glGetUniformLocation(programs.lighting, "windStrength"); //vertexshader
	u.time = glGetUniformLocation(programs.lighting, "time");
	u.winddepth = glGetUniformLocation(programs.depth, "windStrength"); //snow vertexshader
	u.timedepth = glGetUniformLocation(programs.depth, "time");
	u.snowWind = glGetUniformLocation(programs.snow, "windStrength");
	u.snowtime = glGetUniformLocation(programs.snow, "time");
	
	// wind bias (direction)
	u.xbias = glGetUniformLocation(programs.lighting, "x_bias");
	u.zbias = glGetUniformLocation(programs.lighting, "z_bias");
	u.xbiasdepth = glGetUniformLocation(programs.depth, "x_bias");
	u.zbiasdepth = glGetUniformLocation(programs.depth, "z_bias");
	u.xbiassnow = glGetUniformLocation(programs.snow, "x_bias");
	u.zbiassnow = glGetUniformLocation(programs.snow, "z_bias");

	// Snow accumulation uniforms
	GLuint snowAccumMapLoc = glGetUniformLocation(programs.lighting, "snowAccumMap");
	GLuint skyVPLoc = glGetUniformLocation(programs.lighting, "skyVP");
	GLuint snowAmountLoc = glGetUniformLocation(programs.lighting, "snowAmount");
	u.snowAccumMap = snowAccumMapLoc;
	u.skyVP = skyVPLoc;
	u.snowAmount = snowAmountLoc;

	// Fog uniforms
	u.fogDensity = glGetUniformLocation(programs.lighting, "fogDensity");
	u.fogColor = glGetUniformLocation(programs.lighting, "fogColor");


	// Loading a model
	// The terrain object from Gaea is loaded as terrain
	std::string modelPath = "assets/Mesher_LOD3.obj";
	terrain = new Drawable(modelPath);

	

	// Load deer model
	deerModel = new Drawable("assets/deer.obj");
	// deer texture
	deerTexture = loadSOIL("assets/deer_colored.png");

	// load bear model
	bearModel = new Drawable("assets/bear.obj");
	// bear texture
	//bearTexture = loadSOIL("assets/bear_texture.png");
	bearTexture = loadSOIL("assets/brown_bear.png");
	polarbearTexture = loadSOIL("assets/polar_bear.png");
	
	// load wolf model
	wolfModel = new Drawable("assets/wolf.obj");
	// wolf texture
	wolfTexture = loadSOIL("assets/wolf.png");
	
	// load apple tree model
	appleTreeModel = new Drawable("assets/apple.obj");
	// apple tree texture
	appleTexture = loadSOIL("assets/apple.png");

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
	//treeModel1 = new Drawable("assets/apple.obj"); //tree.obj, pinetree.obj, 
	
	pineTreeModel = new Drawable("assets/pinetree.obj");
	// 3 tree textures
	treeDiffuseTex2 = loadTextureRepeat("assets/fir.jpg");

	chrysTexture = loadTextureRepeat("assets/chrys.jpg"); //best?

	trunkTexture = loadTextureRepeat("assets/bark.png");

	needleTexture = loadTextureRepeat("assets/tree2.jpg");

	//// FOREST SYSTEM
	////forest = new Forest(treeModel1, programs.lighting, 100); // 100 trees
	//float scale = SCALING_FACTOR; // 200
	//forest->setTerrainBounds(
	//	-scale / 2, scale / 2,  // X bounds
	//	-scale / 2, scale / 2,  // Z bounds
	//	0.0f, 50.0f,        // Y bounds
	//	1.0f              // scaling factor //? the heightmap is already scaled
	//);
	//forest->loadTerrainBinary("assets/heightmap/terrain_data.bin");
	//// Generate tree positions
	//forest->generate();
	
	// ========================================= APPLE & PINE FOREST ========================================= //
	// ================== APPLE  FOREST ================== //
	// Apple Forest 
	float scale = SCALING_FACTOR;
	// 1. Create Apple Forest (Mode 7, Scale 0.4)
	appleForest = new Forest(appleTreeModel, programs.lighting, 50, 0.7f, 7);
	appleForest->setTerrainBounds(
		-scale / 2, scale / 2,  // X bounds
		-scale / 2, scale / 2,  // Z bounds
		0.0f, 50.0f,        // Y bounds
		1.0f              // scaling factor //? the heightmap is already scaled
	);
	appleForest->loadTerrainBinary("assets/heightmap/terrain_data.bin");
	appleForest->generate(); // Generate first

	// ================== PINE FOREST ================== //
	// pine tree
	pineForest = new Forest(pineTreeModel, programs.lighting, 50, 2.4f, 5);
	pineForest->setTerrainBounds(
		-scale / 2, scale / 2,  // X bounds
		-scale / 2, scale / 2,  // Z bounds
		0.0f, 50.0f,        // Y bounds
		1.0f              // scaling factor //? the heightmap is already scaled
	);
	pineForest->loadTerrainBinary("assets/heightmap/terrain_data.bin");
	// 3. Tell Pine Forest where Apple Forest is to avoid overlap
	pineForest->addExternalPositions(appleForest->instances);
	pineForest->generate();

	// ========================================= BUSH FIELD ========================================= //

	//BUSH
	bushModel = new Drawable("assets/bush2.obj");
	bushes= new BushField(bushModel, programs.lighting, 200);
	bushes->setTerrainBounds(
		-scale / 2, scale / 2,  // X bounds
		-scale / 2, scale / 2,  // Z bounds
		0.0f, 50.0f,        // Y bounds
		1.0f);
	bushes->loadTerrainBinary("assets/heightmap/terrain_data.bin");
	std::vector<glm::vec3> treePositions;
	// Get tree positions from both forests
	for (const auto& t : appleForest->instances)
		treePositions.push_back(t.position);
	for (const auto& t : pineForest->instances)
		treePositions.push_back(t.position);
	bushes->setTreeReferences(treePositions);
	bushes->generate();
	

	// 3 bush textures
	bushTexture1 = loadTextureRepeat("assets/pixel_bush.png");
	bushTexture2 = loadTextureRepeat("assets/bush2.png");
	bushTexture3 = loadTextureRepeat("assets/bush3.png");

	
	// ================================================== CLOUDS  ================================================== //
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

	/* -======--======- FBO CREATION -======--======- */
	// Create depth FBO and texture for shadow mapping
	createDepthFBOAndTexture(depthFBO, depthTexture);

	// snow fbo
	createDepthFBOAndTexture(snowAccumFBO, snowAccumTexture);


	/*==================================== load textures =======================================*/
	snowTexture = loadTextureRepeat("assets/snow.bmp"); 
	snowDetailTexture = loadTextureRepeat("assets/worley_snow.png");
	terrainTexture = loadTextureRepeat("assets/aerial_rocks.bmp");
	 terrainTexture2 = loadTextureRepeat("assets/grass2.bmp");
	 
	 waterTexture = loadTextureRepeat("assets/water.bmp");
	 /*waterTexture = loadSOIL("assets/water.bmp");
	 glBindTexture(GL_TEXTURE_2D, waterTexture);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
	 waterTexture2 = loadTextureRepeat("assets/water2.bmp");

	bottomTexture = loadTextureRepeat("assets/water.bmp");
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

	//snow
	t.snowTex = glGetUniformLocation(programs.lighting, "snowTex");
	t.snowDetailTex = glGetUniformLocation(programs.lighting, "snowDetailTex");
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
	t.appleTex = glGetUniformLocation(programs.lighting, "appleTex");

	t.bushTex1 = glGetUniformLocation(programs.lighting, "bushTex1");
	t.bushTex2 = glGetUniformLocation(programs.lighting, "bushTex2");
	t.bushTex3 = glGetUniformLocation(programs.lighting, "bushTex3");

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

	//snow
	snowSystem = new SnowSystem();
	
	//snowFlakeTexture = loadSOIL("assets/circle.png"); //no longer used, left for future testing (snowflake shape)
	snowSystem = new SnowSystem(10000);
	snowSystem->initialize(programs.snow, snowFlakeTexture);
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
mat4 getSkyViewMatrix() {
	// Camera looking straight down at terrain center
	vec3 skyPos = vec3(0.0f, 200.0f, 0.0f); // High above terrain
	vec3 target = vec3(0.0f, 0.0f, 0.0f);   // Looking at center
	vec3 up = vec3(0.0f, 0.0f, -1.0f);      // Y-axis as "up" for top-down
	return lookAt(skyPos, target, up);
}

mat4 getSkyProjectionMatrix() {
	float halfSize = SCALING_FACTOR / 2.0f; // Match  terrain bounds
	return ortho(-halfSize, halfSize, -halfSize, halfSize, 1.0f, 300.0f);
}
void lighting_pass(mat4 viewMatrix, mat4 projectionMatrix, int screen_width, int screen_height, float currentFogDensity) {
	

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
	//if (currentFogDensity <= 0.5f) { //draw only if low fog
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
	//}
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

	// === NEW: Bind snow accumulation map ===
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, snowAccumTexture);
	glUniform1i(u.snowAccumMap, 9);
	// Calculate and upload sky view-projection matrix (constant)
	mat4 skyVP = getSkyProjectionMatrix() * getSkyViewMatrix();
	glUniformMatrix4fv(u.skyVP, 1, GL_FALSE, &skyVP[0][0]);

	// Calculate snow amount based on accumulation time
	//static float snowLevel = 0.0f;
	snowLevel = max(min(snowAccumulationTime / 50.0f, 1.0f),0);  // 5/50=0.1
	//if (snowingEnabled) {
	//	snowLevel = min(snowAccumulationTime / 30.0f, 1.0f); // 30 seconds to full coverage
	//}
	glUniform1f(u.snowAmount, snowLevel);

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

	//snoww
	// === ADD SNOW TEXTURE HERE ===
	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, snowTexture);
	glUniform1i(t.snowTex, 10);  // Bind to slot 10
	// =============================
	// === ADD SNOW DETAIL TEXTURE ===
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, snowDetailTexture);
	glUniform1i(t.snowDetailTex, 11);  // Bind to slot 11
	// ===============================

	glUniform1i(t.terrainTex, 0);
	glUniform1i(t.terrainTex2, 1);
	glUniform1i(t.waterTex, 2);
	glUniform1i(t.waterTex2, 3);
	glUniform1i(t.bottomTex, 4);
	glUniform1i(t.maskTex, 5);
	glUniform1f(glGetUniformLocation(programs.lighting, "time"), glfwGetTime());

	mat4 terrainM = translate(mat4(), vec3(0.0f, 0.0f, 0.0f)) * scale(mat4(), vec3(SCALING_FACTOR));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &terrainM[0][0]);
	terrain->bind();
	terrain->draw();

	// 4. FOREST (Instanced) - FIXED VERSION
	resetDefaultStates();
	glUniform1i(u.useInstancing, 1);
	glUniform1i(u.useTexture, 5); // Tree mode
	//glUniform1i(u.useTexture, 7);
	glUniform2f(u.uvScale, 1.0f, 1.0f); // Reset UV scale for trees

	//// CRITICAL FIX: Bind tree textures BEFORE drawing
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

	pineForest->draw();
	
	//glUniform1i(u.useTexture, 7); // Same mode as deer/bear
	//glActiveTexture(GL_TEXTURE4);
	//glBindTexture(GL_TEXTURE_2D, appleTexture);
	//glUniform1i(u.diffuseSampler, 4);
	
	// CRITICAL: Unbind terrain textures first to avoid confusion
	/*glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, 0);*/
	/*for (int i = 5; i < 9; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}*/
	// Now bind tree textures in a clean state
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, trunkTexture);
	//glUniform1i(t.trunkTex, 0);

	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, needleTexture);
	//glUniform1i(t.needleTex, 1);

	//glActiveTexture(GL_TEXTURE2);
	//glBindTexture(GL_TEXTURE_2D, treeDiffuseTex2);
	//glUniform1i(t.needleTex2, 2);

	//glActiveTexture(GL_TEXTURE3);
	//glBindTexture(GL_TEXTURE_2D, chrysTexture);
	//glUniform1i(t.needleTex3, 3);

	////pine tree forest
	//glUniform1i(u.useTexture, 5);
	//pineForest->draw();

	resetDefaultStates();
	glUniform1i(u.useInstancing, 1);
	glUniform1i(u.useTexture, 7);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, appleTexture);
	glUniform1i(u.diffuseSampler, 0);
	appleForest->draw();
	
	//forest->draw();

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
	glUniform1i(u.useInstancing, 1);
	glUniform2f(u.uvScale, 1.0f, 1.0f);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, bushTexture2);
	//glUniform1i(u.diffuseSampler, 0);

	//mat4 bushM = translate(mat4(1.0f), vec3(22.0f, 4.0f, 20.0f)) * scale(mat4(1.0f), vec3(0.03f, 0.02f, 0.03f)); //anisotropic scaling 
	//glUniformMatrix4fv(u.M, 1, GL_FALSE, &bushM[0][0]);
	//bushModel->bind();
	//bushModel->draw();
		// CRITICAL FIX: Bind tree textures BEFORE drawing
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bushTexture1);
	glUniform1i(t.bushTex1, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bushTexture2);
	glUniform1i(t.bushTex2, 1);

	// Bind additional needle textures if you have them
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bushTexture3); // fir.jpg
	glUniform1i(t.bushTex3, 2);

	bushes->draw();

	for (int i = 4; i <= 7; i++)
		glDisableVertexAttribArray(i);

	// Only if textureIndex is instanced
	glDisableVertexAttribArray(8);




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


	int gridRes = 1024;
	float minX = -100.0f, maxX = 100.0f;
	float minZ = -100.0f, maxZ = 100.0f;


	//8. DEER 
	//LOAD DEER OBJECT 
	resetDefaultStates();
	glUniform1i(u.useInstancing, 0);
	glUniform1i(u.useTexture, 7); // Bush & Deer mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, deerTexture);
	glUniform1i(u.diffuseSampler, 0);
	deerX = 18.0f;
	deerZ = 24.0f;
	heightData = getHeightDataOnly("assets/heightmap/terrain_data.bin");
	deerY= sampleHeightAt(deerX, deerZ, heightData, gridRes, minX, maxX, minZ, maxZ);
	//
	// cout << "Deer Y position: " << deerY << endl;
	mat4 deerM = translate(mat4(1.0f), vec3(deerX,deerY,deerZ)) * scale(mat4(1.0f), vec3(1.0f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &deerM[0][0]);
	deerModel->bind();
	deerModel->draw();

	//9. BEAR 
	//LOAD BEAR OBJECT
	resetDefaultStates();
	glUniform1i(u.useTexture, 7); // Bush & Deer & Bear mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bearTexture);
	glUniform1i(u.diffuseSampler, 0);
	bearX = -22.0f;
	bearZ = -25.0f;
	bearY = sampleHeightAt(bearX,bearZ, heightData, gridRes, minX, maxX, minZ, maxZ) ;
	//cout << "Bear Y position: " << bearY << endl;
	mat4 bearM = translate(mat4(1.0f), vec3(bearX, bearY, bearZ)) * scale(mat4(1.0f), vec3(2.0f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &bearM[0][0]);
	bearModel->bind();
	bearModel->draw();

	//10.POLAR BEAR
	resetDefaultStates();
	glUniform1i(u.useTexture, 7); // Bush & Deer & Bear mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, polarbearTexture);
	glUniform1i(u.diffuseSampler, 0);
	polarbearX = -22.0f;
	polarbearZ= 39.0f;
	polarbearY = sampleHeightAt(polarbearX, polarbearZ, heightData, gridRes, minX, maxX, minZ, maxZ);
	//cout << "Bear Y position: " << bearY << endl;
	mat4 polarbearM = translate(mat4(1.0f), vec3(polarbearX, polarbearY, polarbearZ)) * scale(mat4(1.0f), vec3(2.0f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &polarbearM[0][0]);
	bearModel->bind();
	bearModel->draw();
	
	//11. WOLF
	resetDefaultStates();
	glUniform1i(u.useTexture, 7); // Bush & Deer & Bear & Wolf mode
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, wolfTexture);
	glUniform1i(u.diffuseSampler, 0);
	wolfX = 30.0f;
	wolfZ = -15.0f;
	wolfY = sampleHeightAt(wolfX, wolfZ, heightData, gridRes, minX, maxX, minZ, maxZ);
	//cout << "Wolf Y position: " << wolfY << endl;
	mat4 wolfM = translate(mat4(1.0f), vec3(wolfX, wolfY, wolfZ)) * scale(mat4(1.0f), vec3(0.35f));
	glUniformMatrix4fv(u.M, 1, GL_FALSE, &wolfM[0][0]);
	wolfModel->bind();
	wolfModel->draw();
	////10. APPLE TREE
	////LOAD APPLE TREE OBJECT
	//resetDefaultStates();
	//glUniform1i(u.useTexture, 7); // Same mode as deer/bear
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, appleTexture);
	//glUniform1i(u.diffuseSampler, 0);
	//appleX = 30.0f;
	//appleZ = 30.0f;
	//appleY = sampleHeightAt(appleX, appleZ, heightData, gridRes, minX, maxX, minZ, maxZ);
	////cout << "Apple Tree Y position: " << appleY << endl;
	//mat4 appleM = translate(mat4(1.0f), vec3(appleX, appleY, appleZ)) * scale(mat4(1.0f), vec3(1.0f));
	//glUniformMatrix4fv(u.M, 1, GL_FALSE, &appleM[0][0]);
	//appleTreeModel->bind();
	//appleTreeModel->draw();
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
	appleForest->draw();
	pineForest->draw();
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
	//bush
	glUniform1i(u.useInstancing, 1);  // Enable instancing
	bushes->draw();
	glUniform1i(u.useInstancing, 0);  // Disable 
	//deer
	mat4 deerM = translate(mat4(1.0f), vec3(deerX, deerY, deerZ)) * scale(mat4(1.0f), vec3(1.0f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &deerM[0][0]);
	deerModel->bind();
	deerModel->draw();
	//bear
	mat4 bearM = translate(mat4(1.0f), vec3(bearX, bearY, bearZ)) * scale(mat4(1.0f), vec3(1.0f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &bearM[0][0]);
	bearModel->bind();
	bearModel->draw();
	////apple tree
	//mat4 appleM = translate(mat4(1.0f), vec3(appleX, appleY, appleZ)) * scale(mat4(1.0f), vec3(1.0f));
	//glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &appleM[0][0]);
	//appleTreeModel->bind();
	//appleTreeModel->draw();
	//polar bear
	mat4 polarbearM = translate(mat4(1.0f), vec3(polarbearX, polarbearY, polarbearZ)) * scale(mat4(1.0f), vec3(2.0f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &polarbearM[0][0]);
	bearModel->bind();
	bearModel->draw();
	//wolf
	mat4 wolfM = translate(mat4(1.0f), vec3(wolfX, wolfY, wolfZ)) * scale(mat4(1.0f), vec3(1.5f));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &wolfM[0][0]);
	wolfModel->bind();
	wolfModel->draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void sky_visibility_pass() {
	cout << "Generating snow accumulation map (one-time calculation)..." << endl;

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, snowAccumFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(programs.depth);

	mat4 skyView = getSkyViewMatrix();
	mat4 skyProj = getSkyProjectionMatrix();
	mat4 skyVP = skyProj * skyView;

	glUniformMatrix4fv(shadowViewProjectionLocation, 1, GL_FALSE, &skyVP[0][0]);

	// Render terrain (blocks snow on steep slopes)
	mat4 terrainM = translate(mat4(), vec3(0.0f, 0.0f, 0.0f)) *
		scale(mat4(), vec3(SCALING_FACTOR));
	glUniformMatrix4fv(shadowModelLocation, 1, GL_FALSE, &terrainM[0][0]);
	terrain->bind();
	terrain->draw();

	// Render trees (they block snow underneath)
	glUniform1i(u.useInstancing, 1);
	appleForest->draw();
	pineForest->draw();
	glUniform1i(u.useInstancing, 0);


	//// Render bushes (optional - they also block some snow)
	//glUniform1i(u.useInstancing, 1);
	//bushes->draw();
	//glUniform1i(u.useInstancing, 0);

	//block lake 
	//use lake mask to block snow accumulation on water surface
	// write lake mask in the buffer as having height? done in shader


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	cout << "Snow accumulation map generated!" << endl;
	snowMapGenerated = true;
}

void updateWind(	GLFWwindow* window,	WindState& wind,	bool& vKeyPressed,	float biasStep = 0.05f) {
	// Toggle wind
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		if (!vKeyPressed) {
			wind.active = !wind.active;
			wind.strength = wind.active ? 2 : 0;

			if (!wind.active) {
				wind.xBias = 0.0f;
				wind.zBias = 0.0f;
			}

			vKeyPressed = true;
			if(wind.active)
				audio.playPreloaded("wind", true);
			else
				audio.stopPreloaded("wind");
		}
	}
	else {
		vKeyPressed = false;
	}

	if (!wind.active) return;

	// X bias
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		wind.xBias += biasStep;
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		wind.xBias -= biasStep;

	// Z bias
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		wind.zBias += biasStep;
	if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS)
		wind.zBias -= biasStep;

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
	snowAccumulationTime = -4.0f; //initially no snow lands, so start negative to delay
	float fogStopedTime = 0.0f;
	//float fogDensity = 0.0f;
	do {
		// Static variables persist between frames
		//static bool vKeyPressed = false;
		//static bool windActive = false;
		// Calculate delta time
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		// Update light and cloud system
		light->update();
		cloudSystem->update(deltaTime);

		// Getting camera information
		camera->update();
		mat4 projectionMatrix = camera->projectionMatrix;
		mat4 viewMatrix = camera->viewMatrix;

		// Check if camera is within 10 units of the wolf 
		if (abs(camera->position.x - wolfX) < 8.0f &&
			abs(camera->position.z - wolfZ) < 8.0f) {
			audio.playPreloaded("wolf_howl", true);
		}
		else {
			audio.stopPreloaded("wolf_howl");
		}
		// Check if camera is within 5 units of the bears
		if ((abs(camera->position.x - bearX) < 7.0f &&	abs(camera->position.z - bearZ) < 7.0f) 
			||
			(abs(camera->position.x - polarbearX) < 7.0f &&	abs(camera->position.z - polarbearZ) < 7.0f)
			){
			audio.playPreloaded("bear_growl", true);
		}
		else {
			audio.stopPreloaded("bear_growl");
		}
		// frustum fit
		light->fitToCameraFrustum(viewMatrix, projectionMatrix);

		// Re-fetch updated light matrices
		mat4 light_proj = light->projectionMatrix;
		mat4 light_view = light->viewMatrix;

		// Now render shadow map
		depth_pass(light_view, light_proj, depthFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//αν σταθερη φωτεινή πηγη δεν εχει νοημα να το κανω καθε frame
		// κάθε δευτερόλεπτο


		// Add cloud on C key press
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
			// Spawn cloud at random position
			vec3 pos = vec3(
				-100.0f + (rand() % 200),  // X: -100 to 100
				35.0f + (rand() % 60),    // Y: 15 to 75
				-100.0f + (rand() % 200)    // Z: -60 to 60
			);
			float size = 4.0f + (rand() % 5); // Size: 4 to 9
			cloudSystem->addCloud(pos, size);
		}
		static bool gKeyPressed = false;  // MOVE OUTSIDE the if statement

		//// WIND ON
		static WindState wind;
		static bool vKeyPressed = false;

		updateWind(window, wind, vKeyPressed);

		// Upload uniforms
		glUseProgram(programs.lighting);
		glUniform1i(u.wind, wind.strength);
		glUniform1f(u.time, currentTime);
		glUniform1f(u.xbias, wind.xBias);
		glUniform1f(u.zbias, wind.zBias);

		glUseProgram(programs.depth);
		glUniform1i(u.winddepth, wind.strength);
		glUniform1f(u.timedepth, currentTime);
		glUniform1f(u.xbiasdepth, wind.xBias);
		glUniform1f(u.zbiasdepth, wind.zBias);

		glUseProgram(programs.snow);
		glUniform1i(u.snowWind, wind.strength);
		glUniform1f(u.snowtime, currentTime);
		glUniform1f(u.xbiassnow, wind.xBias);
		glUniform1f(u.zbiassnow, wind.zBias);

		// Toggle snow system on G key press
		// and add fog
		if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
			if (!gKeyPressed) { // Toggle only on initial press
				
				// Toggle snowing
				snowSystem->toggle();
				bool wasSnowing = snowingEnabled; // Store previous state
				snowingEnabled = snowSystem->isActive();
				// === NEW: Generate snow map ONCE when snow is turned ON ===
				if (snowingEnabled && !snowMapGenerated) {
					// ADD: CLOUDS COVERING THE SKY?
					//spawn 200 clouds at random positions
					for (int i = 0; i < 200; i++) {
						vec3 pos = vec3(
							-80.0f + (rand() % 160),  // X: -80 to 80
							35.0f + (rand() % 60),     // Y: 35 to 95
							-80.0f + (rand() % 160)   // Z: -80 to 80
						);
						float size = 4.0f + (rand() % 5); // Size: 4 to 9
						cloudSystem->addCloud(pos, size);
					}
					sky_visibility_pass(); // Calculate sky visibility
				}
				// Only reset if turning ON after being OFF
				//if (!wasSnowing && snowingEnabled) {
				//	fogAccumulationTime = 0.0f; // Fresh start
				//}
				if(wasSnowing && !snowingEnabled) {
					//clear clouds
					cloudSystem->clearClouds();
				}
				cout << "Snow toggled: " << (snowingEnabled ? "ON" : "OFF") << endl;  // Debug
				gKeyPressed = true;
			}
		}
		else {
			gKeyPressed = false;  // Reset when key released
		}
		
		// Update snow system
		snowSystem->update(deltaTime, camera->position);
		if(wind.active)
			snowSystem->update_velocity(wind.xBias, wind.zBias);
		else
			snowSystem->update_velocity(0.0, 0.0);

	// Track accumulation time and update fog
	if (snowingEnabled) {
		snowAccumulationTime += deltaTime;	
		fogAccumulationTime += deltaTime;
		
	}
	if (!snowingEnabled) {
		fogAccumulationTime -= deltaTime;
	}
	fogAccumulationTime = glm::clamp(fogAccumulationTime, 0.0f, 40.0f);
	fogDensity = fogAccumulationTime / 40.0f;
	// Update fog based on snowing state
	//fogDensity = snowingEnabled ? min(fogAccumulationTime / 40.0f, 1.0f) : max(fogDensity -(fogStopedTime)/40.0f,0.0f); // Full fog when snowing, no fog otherwise
	//
	vec3 fogColor = vec3(0.8f, 0.85f, 0.9f); // Light grayish-blue fog color

	// Rendering the scene from light's perspective when F1 is pressed

	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
		//lighting_pass(light_view, light_proj);
		lighting_pass(light_view, light_proj, fb_width, fb_height, fogDensity);
	}
	else {
		// Render the scene from camera's perspective
		//lighting_pass(viewMatrix, projectionMatrix);
		lighting_pass(viewMatrix, projectionMatrix, fb_width, fb_height, fogDensity);
	}



	// Upload fog uniforms to shader
	glUseProgram(programs.lighting);
	glUniform1f(u.fogDensity, fogDensity);
	glUniform3f(u.fogColor, fogColor.r, fogColor.g, fogColor.b);

		// In lighting_pass(), after rendering everything else:
		snowSystem->render(viewMatrix, projectionMatrix);

		// Render clouds
		glUseProgram(programs.lighting);
		glUniformMatrix4fv(u.V, 1, GL_FALSE, &viewMatrix[0][0]);
		glUniformMatrix4fv(u.P, 1, GL_FALSE, &projectionMatrix[0][0]);
		cloudSystem->render(viewMatrix, projectionMatrix);
		
		//if x is pressed delete all clouds 
		// clear clouds
		// delete fog
		// stop snow turn it off if on if already off do nothing
		if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
			//delete cloudSystem;
			//cloudSystem->clearClouds();
		}
		
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
	glClearColor(0.8f, 0.85f, 0.9f, 1.0f);
	//glClearColor(0.27f, 0.537f, 0.725f, 0.0f);

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
		audio.preload("wind", "sfx/wind.mp3");
		audio.preload("wolf_howl", "sfx/wolf_howl.mp3");
		audio.preload("bear_growl", "sfx/bear.mp3");
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

//height helpers
float sampleHeightAt(float x,float z,const std::vector<float>& heightData,int gridResolution,
	float minX, float maxX,float minZ, float maxZ){
	if (heightData.empty() || gridResolution <= 0)
		return 0.0f; // fallback to ground level

	// Convert world coords to normalized [0,1]
	float u = (x - minX) / (maxX - minX);
	float v = (z - minZ) / (maxZ - minZ);

	u = glm::clamp(u, 0.0f, 1.0f);
	v = glm::clamp(v, 0.0f, 1.0f);

	// Convert to grid indices
	int ix = static_cast<int>(u * (gridResolution - 1));
	int iz = static_cast<int>(v * (gridResolution - 1));

	ix = glm::clamp(ix, 0, gridResolution - 1);
	iz = glm::clamp(iz, 0, gridResolution - 1);

	// Sample height from heightmap
	return heightData[iz * gridResolution + ix];
}
std::vector<float> getHeightDataOnly(const std::string& filePath) {
	std::ifstream file(filePath, std::ios::binary);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open file " << filePath << std::endl;
		return {}; // Return empty vector on failure
	}

	try {
		int gridResolution;
		// 1. Read the first integer for resolution
		file.read(reinterpret_cast<char*>(&gridResolution), sizeof(int));

		// 2. Skip the header metadata (7 floats: scaling, minX, maxX, minZ, maxZ, minY, maxY)
		// Each float is 4 bytes, so we skip 28 bytes.
		file.seekg(7 * sizeof(float), std::ios::cur);

		// 3. Prepare the vector
		size_t numElements = static_cast<size_t>(gridResolution) * gridResolution;
		std::vector<float> heights(numElements);

		// 4. Read ONLY the height block
		file.read(reinterpret_cast<char*>(heights.data()), numElements * sizeof(float));

		if (file.fail()) {
			throw std::runtime_error("Failed to read height data from stream.");
		}

		file.close();
		return heights; // Success!
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		file.close();
		return {};
	}
}
