///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}


/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** The code in the methods BELOW is for preparing and     ***/
/*** rendering the 3D replicated scenes.                    ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"wall");

	bReturn = CreateGLTexture(
		"textures/tilesf2.jpg",
		"lampshade");

	bReturn = CreateGLTexture(
		"textures/stone.jpg",
		"stone");

	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"floor");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f); // Neutral ambient color
	metalMaterial.ambientStrength = 0.5f;
	metalMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f); // Neutral diffuse color
	metalMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // Stronger specular reflection
	metalMaterial.shininess = 128.0f; // High shininess for metal
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL floorMaterial;
	floorMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.1f); // Adjusted ambient color
	floorMaterial.ambientStrength = 0.5f;
	floorMaterial.diffuseColor = glm::vec3(0.6f, 0.4f, 0.2f); // Adjusted diffuse color
	floorMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); // Specular reflection for highlights
	floorMaterial.shininess = 32.0f; // Shininess for specular highlights
	floorMaterial.tag = "floor";
	m_objectMaterials.push_back(floorMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.5f); // Light blue ambient color to simulate light passing through
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.7f); // Light blue diffuse color
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 1.0f); // Strong specular reflection
	glassMaterial.shininess = 64.0f; // High shininess for glass
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f); // Neutral ambient color
	tableMaterial.ambientStrength = 0.5f;
	tableMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f); // Neutral diffuse color
	tableMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f); // Neutral specular color
	tableMaterial.shininess = 32.0f; // Moderate shininess
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);

	OBJECT_MATERIAL wallMaterial;
	wallMaterial.ambientColor = glm::vec3(0.50f, 0.79f, 0.61f); // Light green color
	wallMaterial.ambientStrength = 0.3f;
	wallMaterial.diffuseColor = glm::vec3(0.50f, 0.79f, 0.61f);
	wallMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Slight specular highlight
	wallMaterial.shininess = 16.0f; // Moderate shininess
	wallMaterial.tag = "wall";
	m_objectMaterials.push_back(wallMaterial);

	OBJECT_MATERIAL bookCoverMaterial;
	bookCoverMaterial.ambientColor = glm::vec3(0.3f, 0.2f, 0.1f); // Adjust the color for a book cover
	bookCoverMaterial.ambientStrength = 0.5f;
	bookCoverMaterial.diffuseColor = glm::vec3(0.5f, 0.4f, 0.3f); // Adjust the diffuse color
	bookCoverMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f); // Specular highlights
	bookCoverMaterial.shininess = 32.0f; // Moderate shininess
	bookCoverMaterial.tag = "bookCover";
	m_objectMaterials.push_back(bookCoverMaterial);

	OBJECT_MATERIAL grayBlackLeather;
	grayBlackLeather.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f); // Dark gray ambient color
	grayBlackLeather.ambientStrength = 0.6f;
	grayBlackLeather.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Gray-black diffuse color
	grayBlackLeather.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Low specular highlights for a matte look
	grayBlackLeather.shininess = 16.0f; // Moderate shininess
	grayBlackLeather.tag = "grayBlackLeather";
	m_objectMaterials.push_back(grayBlackLeather);

	OBJECT_MATERIAL airFreshenerMaterial;
	airFreshenerMaterial.ambientColor = glm::vec3(0.4f, 0.5f, 0.4f); // Light green ambient color
	airFreshenerMaterial.ambientStrength = 0.6f; // Increase ambient strength for better visibility
	airFreshenerMaterial.diffuseColor = glm::vec3(0.5f, 0.7f, 0.5f); // Green diffuse color
	airFreshenerMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); // Brighter specular highlights
	airFreshenerMaterial.shininess = 32.0f; // Moderate shininess
	airFreshenerMaterial.tag = "airFreshener";
	m_objectMaterials.push_back(airFreshenerMaterial);
}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting in the shader
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Ambient Light
	m_pShaderManager->setVec3Value("ambientLight.color", 0.3f, 0.3f, 0.3f); // Slightly increased ambient light

	// Directional Light (e.g., Sunlight)
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.2f, -1.0f, -0.3f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.3f, 0.3f, 0.3f); // Enhanced ambient light
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.6f, 0.6f, 0.6f);  // Softer diffuse light
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.5f, 0.5f, 0.5f); // Neutral specular light
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point Light (e.g., Lamp)
	m_pShaderManager->setVec3Value("pointLights[0].position", 4.0f, 12.65f, -2.0f); // Position near the lamp shade
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.2f, 0.2f, 0.2f);   // Softer ambient light
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.5f, 0.5f, 0.5f);   // Softer diffuse light
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.8f, 0.8f, 0.8f);  // Bright specular light
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point Light 2 (e.g., Additional Light)
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.5f, 1.0f, 0.0f); // Position near the table
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.1f, 0.1f);   // Softer ambient light
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.4f, 0.4f, 0.4f);   // Neutral diffuse light
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.5f, 0.5f, 0.5f);  // Moderate specular light
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTriangularPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderAirFreshner();
	RenderBackdrop();
	RenderBook();
	RenderFloor();
	RenderGlassesCase();
	RenderLamp();
	RenderTable();
}

/***********************************************************
 *  RenderAirFreshner()
 *
 *  This method is called to render the shapes for the air
 *  freshner object.
 ***********************************************************/
void SceneManager::RenderAirFreshner()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Set the XYZ scale for the air freshener
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.2f); // Adjust the scale to match the dimensions of an air freshener

	// Set the XYZ position for the air freshener (on the table or where desired)
	positionXYZ = glm::vec3(2.75f, 7.1f, 5.0f); // Adjust the position to place the air freshener on the table

	// Set the transformations into memory to be used on the air freshener
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the material for the air freshener
	SetShaderMaterial("airFreshener");
	SetShaderColor(0.5f, 0.7f, 0.5f, 1.0f); // Setting a medium green color
	// Draw the mesh for the air freshener with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // Use the tapered cylinder mesh to represent the air freshener
}


/***********************************************************
 *  RenderBackdrop()
 *
 *  This method is called to render the backdrop for the
 *  scene.
 ***********************************************************/
void SceneManager::RenderBackdrop() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 30.0f);

	// set the XYZ rotation for the background plane mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the background plane mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -10.0f);

	// set the transformations into memory to be used on the background plane mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.50f, 0.79f, 0.61f, 1.00f);
	SetShaderTexture("wall");
	SetTextureUVScale(4.0, 4.0); // Tiling texture across background wall
	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}


/***********************************************************
 *  RenderBook()
 *
 *  This method is called to render the shapes for the book
 *  object.
 ***********************************************************/
void SceneManager::RenderBook() { 
	// declare the variables for the transformations 
	glm::vec3 scaleXYZ; 
	float XrotationDegrees = 0.0f; 
	float YrotationDegrees = -195.0f; // Rotate the book slightly for a natural look 
	float ZrotationDegrees = 0.0f; 
	glm::vec3 positionXYZ; 
	
	// set the XYZ scale for the book 
	scaleXYZ = glm::vec3(4.0f, 0.4f, 5.5); // Adjust the scale to match a book's dimensions 
	
	// set the XYZ position for the book (on top of the table) 
	positionXYZ = glm::vec3(-1.5f, 7.2f, 4.0f); // Adjust the position to place the book on the table 
	
	// set the transformations into memory to be used on the book 
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ); 
	
	// Apply the texture for the book 
	SetShaderMaterial("bookCover"); 
	
	// draw the mesh for the book with transformation values 
	m_basicMeshes->DrawBoxMesh(); // Use a box mesh to represent the book 
}


/***********************************************************
 *  RenderFloor()
 *
 *  This method is called to render the floor for the
 *  scene.
 ***********************************************************/
void SceneManager::RenderFloor() 
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/
	// set the XYZ scale for the floor plane mesh
	scaleXYZ = glm::vec3(50.0f, 1.0f, 30.0f);

	// set the XYZ rotation for the floor plane mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the floor plane mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the floor plane mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.21f, 0.18f, 0.17f, 1.00f);
	SetShaderTexture("floor");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("floor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

/***********************************************************
 *  RenderGlassesCase()
 *
 *  This method is called to render the shapes for the glasses
 *  case object.
 ***********************************************************/
void SceneManager::RenderGlassesCase()
{
	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 90.0f; // Rotate 90 degrees to place on its side
	float YrotationDegrees = 60.0f;
	float ZrotationDegrees = 180.0f;
	glm::vec3 positionXYZ;

	// Set the XYZ scale for the glasses case
	scaleXYZ = glm::vec3(1.5f, 4.5f, 1.5f); // Adjust the scale to match the dimensions of a glasses case

	// Set the XYZ position for the glasses case (on the table)
	positionXYZ = glm::vec3(-1.0f, 7.1f, -2.0f); // Adjust the position to place the glasses case on the table

	// Set the transformations into memory to be used on the glasses case
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply the leather material for the glasses case
	SetShaderMaterial("grayBlackLeather");
	SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f); // Setting a dark gray-black color
	// Draw the mesh for the glasses case with transformation values
	m_basicMeshes->DrawTriangularPrismMesh(); // Use the triangular prism mesh to represent the glasses case
}

/***********************************************************
 *  RenderLamp()
 *
 *  This method is called to render the shapes for the lamp
 *  object.
 ***********************************************************/
void SceneManager::RenderLamp()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the complex base cylinder mesh (1 of 5)
	scaleXYZ = glm::vec3(2.0f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 7.0f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.45f, 0.38f, 0.34f, 1.00f);
	SetShaderTexture("stone");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// set the color for the next draw command
	//SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	//m_basicMeshes->DrawCylinderMeshLines();
	/****************************************************************/
	// set the XYZ scale for the complex base cylinder mesh (2 of 5)
	scaleXYZ = glm::vec3(1.75f, 1.25f, 1.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 7.5f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.45f, 0.38f, 0.34f, 1.00f);
	SetShaderTexture("stone");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();
	// set the color for the next draw command
	//SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	//m_basicMeshes->DrawTaperedCylinderMeshLines();
	/****************************************************************/

	// set the XYZ scale for the complex base cylinder mesh (3 of 5)
	scaleXYZ = glm::vec3(1.0f, 0.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 8.25f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.45f, 0.38f, 0.34f, 1.00f);
	SetShaderTexture("stone");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// set the color for the next draw command
	//SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	//m_basicMeshes->DrawCylinderMeshLines();
	/****************************************************************/

	// set the XYZ scale for the complex base cylinder mesh (4 of 5)
	scaleXYZ = glm::vec3(0.75f, 0.80f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 8.75f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.45f, 0.38f, 0.34f, 1.00f);
	SetShaderTexture("stone");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// set the color for the next draw command
	// SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	//m_basicMeshes->DrawCylinderMeshLines();
	/****************************************************************/

	// set the XYZ scale for the complex base cylinder mesh (5 of 5)
	scaleXYZ = glm::vec3(1.00f, 0.10f, 1.00f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 9.55f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.45f, 0.38f, 0.34f, 1.00f);
	SetShaderTexture("stone");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// set the color for the next draw command
	// SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	// m_basicMeshes->DrawCylinderMeshLines();
	/****************************************************************/
	/*** Stem of Lamp ***********************************************/
	/****************************************************************/

	// set the XYZ scale for the lamp stem (cylinder)
	scaleXYZ = glm::vec3(0.15f, 5.00f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 9.65f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.45f, 0.38f, 0.34f, 1.00f);
	SetShaderTexture("stone");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// set the color for the next draw command
	// SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	// m_basicMeshes->DrawCylinderMeshLines();
	/****************************************************************/

	/****************************************************************/
	/***  Lamp Shade  ***********************************************/
	/****************************************************************/

	// set the XYZ scale for the lamp shade (half sphere)
	scaleXYZ = glm::vec3(2.75f, 3.50f, 2.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 12.65f, -2.0f);

	// set the transformations into memory to be used on the mesh
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(0.52f, 0.37f, 0.65f, 1.00f);  // light violet
	SetShaderTexture("lampshade");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawHalfSphereMesh();
	// set the color for the next draw command
	// SetShaderColor(0, 0, 0, 1);
	// draw the plane shape outline
	// m_basicMeshes->DrawHalfSphereMeshLines();
	/****************************************************************/

}

/***********************************************************
 *  RenderTable()
 *
 *  This method is called to render the shapes for the glasses
 *  case object.
 ***********************************************************/
void SceneManager::RenderTable()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the table top (cylinder)
	scaleXYZ = glm::vec3(8.0f, 0.2f, 8.0f);

	// set the XYZ rotation for the table top
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the table top
	positionXYZ = glm::vec3(0.0f, 6.9f, 0.0f);

	// set the transformations into memory to be used on the table top
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor to black/gray color 
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.00f); // adjust the values for the desired shade	
	SetShaderMaterial("table");

	// draw the table top with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the table legs (cylinders)
	scaleXYZ = glm::vec3(0.1f, 6.9f, 0.1f);

	// set the XYZ rotation for the table legs
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// positions for the four legs (directly under the table top, touching the floor) 
	glm::vec3 legPositions[4] = { 
		glm::vec3(5.0f, 0.0f, 5.0f), // half of the table top height 
		glm::vec3(-5.0f, 0.0f, 5.0f), 
		glm::vec3(5.0f, 0.0f, -5.0f), 
		glm::vec3(-5.0f, 0.0f, -5.0f) 
	};

	for (int i = 0; i < 4; ++i)
	{
		// set the XYZ position for each leg
		positionXYZ = legPositions[i];

		// set the transformations into memory to be used on the table legs
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		// SetShaderColor to black/gray color 
		SetShaderColor(0.15f, 0.15f, 0.15f, 1.00f); // same color as the top		
		SetShaderMaterial("table");

		// draw the table leg with transformation values
		m_basicMeshes->DrawCylinderMesh();
	}
}
