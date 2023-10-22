#include <iostream>
#include <cstdlib>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

namespace {
	const char* const WINDOW_TITLE = "Pyramid Test";	//Window Title

	const int WINDOW_HEIGHT = 600;
	const int WINDOW_WIDTH = 800;

	struct GLMesh {
		GLuint vao[10];
		GLuint vbos[20];
		GLuint nVertices[20];
};
	//Main GLFW window
	GLFWwindow* gWindow = nullptr;
	//Triangle mesh data
	GLMesh gMesh;
	//Shader program
	GLuint gProgramId;
	//Textures
	GLuint houseTextureId;
	GLuint floorTextureId;
	GLuint tissueTextureId;
	GLuint watchTextureId;
	GLuint capTextureId;
	GLuint watchFaceTextureId;
	GLuint bottleTextureId;

	//Camera
	float cameraSpeed = 2.0f;
	Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	bool viewProjection = true;
	glm::vec3 gCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 gCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 gCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	//Timing
	float gDeltaTime = 0.0f;	//Time between current and last frame
	float gLastFrame = 0.0f;

	//Lighting variables
	glm::vec3 gLightColor(0.85f, 0.85f, 0.86f);
	glm::vec3 gLightColorB(0.98f, 0.85f, 0.95f);
	glm::vec3 gLightPosition(20.0f, 15.0f, -15.0f);
	glm::vec3 gLightPositionB(20.0f, 30.0f, 30.0f);
	glm::vec3 gLightScale(1.0f);
	glm::vec3 gLightScaleB(0.1f);
	glm::vec2 gUVScale(0.5f, 0.5f);
	glm::vec2 gUVScaleB(0.05f, 0.05f);

	const double pi = 3.14159265358979323846;
}

//Functions to intitialize, set window size and draw on screen
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* fileName, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

//Vertex shader source code
const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);

//Fragment shader source code
const GLchar* fragmentShaderSource = GLSL(440,
	in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for light color, light position, and camera/view position
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;

uniform vec3 lightColorB;
uniform vec3 lightPosB;
uniform vec3 viewPositionB;

uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	//Calculate Ambient lighting*/
	float ambientStrength = 0.3f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse = impact * lightColor; // Generate diffuse light color

	//Calculate Specular lighting*/
	float specularIntensity = 0.1f; // Set specular light strength
	float highlightSize = 16.0f; // Set specular highlight size
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector

	//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;

	//Calculate Ambient lighting*/
	float ambientStrengthB = 0.5f; // Set ambient or global lighting strength
	vec3 ambientB = ambientStrengthB * lightColorB; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 normB = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirectionB = normalize(lightPosB - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impactB = max(dot(normB, lightDirectionB), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuseB = impactB * lightColorB; // Generate diffuse light color

	//Calculate Specular lighting*/
	float specularIntensityB = 0.2f; // Set specular light strength
	float highlightSizeB = 16.0f; // Set specular highlight size
	vec3 viewDirB = normalize(viewPositionB - vertexFragmentPos); // Calculate view direction
	vec3 reflectDirB = reflect(-lightDirectionB, normB);// Calculate reflection vector

	//Calculate specular component
	float specularComponentB = pow(max(dot(viewDirB, reflectDirB), 0.0), highlightSizeB);
	vec3 specularB = specularIntensityB * specularComponentB * lightColorB;

	// Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	// Calculate phong result
	vec3 phong = ((ambient + diffuse + specular) + (ambientB + diffuseB + specularB)) * textureColor.xyz;

	fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}


int main(int argc, char* argv[]) {
	if (!UInitialize(argc, argv, &gWindow)) {
		return EXIT_FAILURE;
	}

	//Create mesh
	UCreateMesh(gMesh);		//Calls function to create vbo

	//Create shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId)) {
		return EXIT_FAILURE;
	}

	//Load texture
	const char* houseTexFileName = "textures/housetexture.jpg";
	const char* floorTexFileName = "textures/blankback.jpg";
	const char* tissueTexFileName = "textures/tissuetexture.jpg";
	const char* watchTexFileName = "textures/watchtexture.jpg";
	const char* bottleTexFileName = "textures/bottletexture.jpg";
	const char* capTexFileName = "textures/captexture.jpg";
	const char* watchFaceTexFileName = "textures/watchfacetexture.jpg";

	if (!UCreateTexture(houseTexFileName, houseTextureId))
	{
		cout << "Failed to load texture " << houseTexFileName << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(floorTexFileName, floorTextureId))
	{
		cout << "Failed to load texture " << floorTexFileName << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(tissueTexFileName, tissueTextureId))
	{
		cout << "Failed to load texture " << tissueTexFileName << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(watchTexFileName, watchTextureId))
	{
		cout << "Failed to load texture " << watchTexFileName << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(bottleTexFileName, bottleTextureId))
	{
		cout << "Failed to load texture " << bottleTexFileName << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(watchFaceTexFileName, watchFaceTextureId))
	{
		cout << "Failed to load texture " << watchFaceTexFileName << endl;
		return EXIT_FAILURE;
	}

	if (!UCreateTexture(capTexFileName, capTextureId))
	{
		cout << "Failed to load texture " << capTexFileName << endl;
		return EXIT_FAILURE;
	}

	// Tell OpenGL for each sampler which texture unit it belongs to
	glUseProgram(gProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	//Set background color to black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//Render loop
	while (!glfwWindowShouldClose(gWindow)) {
		//Frame timing
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		UProcessInput(gWindow);
		URender();
		glfwPollEvents();
	}

	//Release mesh data
	UDestroyMesh(gMesh);

	//Release shader program
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS);		//Successfully terminate program
}


//Initialize GLFW, GLEW and create a new window
bool UInitialize(int argc, char* argv[], GLFWwindow** window) {
	//GLFW intialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	//GLFW window creation
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	//capture mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	//GLEW intialize
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult) {
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	//Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}

//Processes input, determines whether relevant keys are hit and responds accordingly
void UProcessInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	float cameraOffSet = cameraSpeed * gDeltaTime;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		viewProjection = !viewProjection;
}

//Respond to window resize
void UResizeWindow(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (gFirstMouse) {
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos;

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}


// slow down or speed up scroll speed
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	if (yoffset > 0 && cameraSpeed < 0.1f)
		cameraSpeed += 0.01f;

	if (yoffset < 0 && cameraSpeed >0.01f)
		cameraSpeed -= 0.01f;
}

//Function to render a frame
void URender() {
	//Enable z depth (for 3D objects)
	glEnable(GL_DEPTH_TEST);

	//Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Set shader to use
	glUseProgram(gProgramId);

	//camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	glm::mat4 projection;

	// Create a perspective projection
	if (viewProjection) {
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		float scale = 120;
		projection = glm::ortho((800.0f / scale), -(800.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
	}

	//LIGHT SOURCE BUSINESS
	// Reference matrix uniforms from the Cube Shader program for the cube color, light color, light position, and camera position
	GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms.
	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	GLuint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

	//SECOND LIGHT SOURCE
	//// Reference matrix uniforms from the Cube Shader program for the cube color, light color, light position, and camera position
	//GLint objectColorLocB = glGetUniformLocation(gProgramId, "objectColorB");
	//GLint lightColorLocB = glGetUniformLocation(gProgramId, "lightColorB");
	//GLint lightPositionLocB = glGetUniformLocation(gProgramId, "lightPosB");
	//GLint viewPositionLocB = glGetUniformLocation(gProgramId, "viewPositionB");

	//// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms.
	//glUniform3f(lightColorLocB, gLightColorB.r, gLightColorB.g, gLightColorB.b);
	//glUniform3f(lightPositionLocB, gLightPositionB.x, gLightPositionB.y, gLightPositionB.z);
	//const glm::vec3 cameraPositionB = gCamera.Position;
	//glUniform3f(viewPositionLocB, cameraPositionB.x, cameraPositionB.y, cameraPositionB.z);

	//GLuint UVScaleLocB = glGetUniformLocation(gProgramId, "uvScaleB");
	//glUniform2fv(UVScaleLocB, 1, glm::value_ptr(gUVScaleB));

	//PYRAMID
	glm::mat4 scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	glm::mat4 translation = glm::translate(glm::vec3(0.25f, -0.5f, -0.25f));
	glm::mat4 model = translation * rotation * scale;
	// Retrives and passes transformations to shader program
	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, houseTextureId);

	//Activate VBOs in vao
	glBindVertexArray(gMesh.vao[0]);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);

	//CUBE
	//Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, houseTextureId);

	scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.25f, -0.75f, 0.0f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[1]);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);

	//PLANE(FLOOR)
	//Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorTextureId);

	scale = glm::scale(glm::vec3(10.0f, 10.0f, 10.0f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(0.0f, 4.0f, 0.0f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[2]);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[2]);

	//TISSUE BOX
	//Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tissueTextureId);

	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(1.5f, -0.5f, 0.5f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[3]);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);

	//BOTTLE BODY
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bottleTextureId);

	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(1.3f, 0.3f, -0.4f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[4]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, gMesh.nVertices[4]);

	//TOP OF BOTTLE
	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(1.3f, 0.3f, -1.15f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[5]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, gMesh.nVertices[5]);

	//BOTTOM OF BOTTLE
	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(1.3f, 0.3f, 0.35f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[6]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, gMesh.nVertices[6]);

	//CAP BODY
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, capTextureId);

	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(1.3f, 0.3f, 0.388));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[7]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, gMesh.nVertices[7]);

	//CAP TOP
	scale = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(glm::vec3(1.3f, 0.3f, 0.4255f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[8]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, gMesh.nVertices[8]);

	//WATCH (HAND 1)
	//Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, watchTextureId);

	scale = glm::scale(glm::vec3(1.0f, 1.0f, 0.75f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(-0.3f, -0.97f, -0.32f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[9]);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[9]);

	//WATCH (HAND 2)
	scale = glm::scale(glm::vec3(1.0f, 1.0f, 0.5f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	translation = glm::translate(glm::vec3(-0.3f, -0.97f, 0.68f));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[9]);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[9]);

	//WATCH FACE BODY
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, watchTextureId);

	scale = glm::scale(glm::vec3(2.0f, 1.0f, 2.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(-0.3f, -0.955f, -0.22));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[7]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, gMesh.nVertices[7]);

	//WATCH FACE TOP
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, watchFaceTextureId);

	scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
	rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	translation = glm::translate(glm::vec3(-0.3f, -0.919f, -0.22));
	model = translation * rotation * scale;
	modelLoc = glGetUniformLocation(gProgramId, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glBindVertexArray(gMesh.vao[8]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, gMesh.nVertices[8]);

	//Deactive the VAO
	glBindVertexArray(0);

	//GLFW swap buffers and poll events
	glfwSwapBuffers(gWindow);
}

//Implement UCreateMesh
void UCreateMesh(GLMesh &mesh){
	// Generate cylinder geometry
	const float pi = glm::pi<float>();
	const int numSegments = 20; // The number of segments that make up the cylinder
	const float radius = 0.3f;  // Radius of the cylinder
	const float height = 1.5f;  // Height of the cylinder

	// Arrays to store vertex, normal, and texture coordinate data
	std::vector<glm::vec3> sideVertices;
	std::vector<glm::vec3> sideNormals;
	std::vector<glm::vec2> sideTexCoords;

	// Generate side vertices, normals, and texture coordinates
	for (int i = 0; i < numSegments; ++i) {
		float theta1 = 2.0f * pi * static_cast<float>(i) / numSegments;
		float theta2 = 2.0f * pi * static_cast<float>(i + 1) / numSegments;

		// Calculate vertex positions for the side
		glm::vec3 topVertex1(radius * cos(theta1), height / 2.0f, radius * sin(theta1));
		glm::vec3 topVertex2(radius * cos(theta2), height / 2.0f, radius * sin(theta2));
		glm::vec3 bottomVertex1(radius * cos(theta1), -height / 2.0f, radius * sin(theta1));
		glm::vec3 bottomVertex2(radius * cos(theta2), -height / 2.0f, radius * sin(theta2));

		// Calculate normal vectors for the side (lateral) surface
		glm::vec3 sideNormal1 = glm::normalize(glm::vec3(topVertex1.x, 0.0f, topVertex1.z));
		glm::vec3 sideNormal2 = glm::normalize(glm::vec3(topVertex2.x, 0.0f, topVertex2.z));

		// Calculate texture coordinates for the side
		float u1 = static_cast<float>(i) / numSegments;
		float u2 = static_cast<float>(i + 1) / numSegments;

		// Add vertices, normals, and texture coordinates to the arrays
		sideVertices.push_back(topVertex1);
		sideVertices.push_back(bottomVertex1);
		sideVertices.push_back(topVertex2);
		sideVertices.push_back(bottomVertex2);

		sideNormals.push_back(sideNormal1);
		sideNormals.push_back(sideNormal1);
		sideNormals.push_back(sideNormal2);
		sideNormals.push_back(sideNormal2);

		sideTexCoords.push_back(glm::vec2(u1, 0.0f));
		sideTexCoords.push_back(glm::vec2(u1, 1.0f));
		sideTexCoords.push_back(glm::vec2(u2, 0.0f));
		sideTexCoords.push_back(glm::vec2(u2, 1.0f));
	}

	// Arrays to store vertex, normal, and texture coordinate data for the top and bottom circles
	std::vector<glm::vec3> circleVertices;
	std::vector<glm::vec3> circleNormals;
	std::vector<glm::vec2> circleTexCoords;

	for (int i = 0; i < numSegments; ++i) {
		float theta = 2.0f * pi * static_cast<float>(i) / numSegments;
		float x = radius * cos(theta);
		float y = radius * sin(theta);

		// Calculate vertex positions for the circle
		glm::vec3 vertex(x, y, 0.0f);

		// Calculate normal vectors for the circle (0, 0, 1) since it's in the XY plane
		glm::vec3 normal(0.0f, 0.0f, 1.0f);

		// Calculate texture coordinates
		float u = static_cast<float>(i) / numSegments;
		float v = 0.0f; // Texture coordinate in this case can be set to 0

		// Add vertices, normals, and texture coordinates to the arrays
		circleVertices.push_back(vertex);
		circleNormals.push_back(normal);
		circleTexCoords.push_back(glm::vec2(u, v));
	}

	// Arrays to store vertex, normal, and texture coordinate data for the top and bottom circles
	std::vector<glm::vec3> circleVerticesB;
	std::vector<glm::vec3> circleNormalsB;
	std::vector<glm::vec2> circleTexCoordsB;

	for (int i = 0; i < numSegments; ++i) {
		float theta = 2.0f * pi * static_cast<float>(i) / numSegments;
		float x = radius * cos(theta);
		float y = radius * sin(theta);

		// Calculate vertex positions for the circle
		glm::vec3 vertex(x, y, 0.0f);

		// Calculate normal vectors for the circle (0, 0, 1) since it's in the XY plane
		glm::vec3 normal(0.0f, 0.0f, 1.0f);

		// Calculate texture coordinates
		float u = static_cast<float>(i) / numSegments;
		float v = 0.0f; // Texture coordinate in this case can be set to 0

		// Add vertices, normals, and texture coordinates to the arrays
		circleVerticesB.push_back(vertex);
		circleNormalsB.push_back(normal);
		circleTexCoordsB.push_back(glm::vec2(u, v));
	}

	//CAP
	// Generate cylinder geometry
	const int numSegmentsB = 20; // The number of segments that make up the cylinder
	const float radiusB = 0.075f;  // Radius of the cylinder
	const float heightB = 0.075f;  // Height of the cylinder

	// Arrays to store vertex, normal, and texture coordinate data
	std::vector<glm::vec3> sideVerticesB;
	std::vector<glm::vec3> sideNormalsB;
	std::vector<glm::vec2> sideTexCoordsB;

	// Generate side vertices, normals, and texture coordinates
	for (int i = 0; i < numSegmentsB; ++i) {
		float theta1 = 2.0f * pi * static_cast<float>(i) / numSegmentsB;
		float theta2 = 2.0f * pi * static_cast<float>(i + 1) / numSegmentsB;

		// Calculate vertex positions for the side
		glm::vec3 topVertex1(radiusB * cos(theta1), heightB / 2.0f, radiusB * sin(theta1));
		glm::vec3 topVertex2(radiusB * cos(theta2), heightB / 2.0f, radiusB * sin(theta2));
		glm::vec3 bottomVertex1(radiusB * cos(theta1), -heightB / 2.0f, radiusB * sin(theta1));
		glm::vec3 bottomVertex2(radiusB * cos(theta2), -heightB / 2.0f, radiusB * sin(theta2));

		// Calculate normal vectors for the side (lateral) surface
		glm::vec3 sideNormal1 = glm::normalize(glm::vec3(topVertex1.x, 0.0f, topVertex1.z));
		glm::vec3 sideNormal2 = glm::normalize(glm::vec3(topVertex2.x, 0.0f, topVertex2.z));

		// Calculate texture coordinates for the side
		float u1 = static_cast<float>(i) / numSegmentsB;
		float u2 = static_cast<float>(i + 1) / numSegmentsB;

		// Add vertices, normals, and texture coordinates to the arrays
		sideVerticesB.push_back(topVertex1);
		sideVerticesB.push_back(bottomVertex1);
		sideVerticesB.push_back(topVertex2);
		sideVerticesB.push_back(bottomVertex2);

		sideNormalsB.push_back(sideNormal1);
		sideNormalsB.push_back(sideNormal1);
		sideNormalsB.push_back(sideNormal2);
		sideNormalsB.push_back(sideNormal2);

		sideTexCoordsB.push_back(glm::vec2(u1, 0.0f));
		sideTexCoordsB.push_back(glm::vec2(u1, 1.0f));
		sideTexCoordsB.push_back(glm::vec2(u2, 0.0f));
		sideTexCoordsB.push_back(glm::vec2(u2, 1.0f));
	}

	// Arrays to store vertex, normal, and texture coordinate data for the top and bottom circles
	std::vector<glm::vec3> circleVerticesC;
	std::vector<glm::vec3> circleNormalsC;
	std::vector<glm::vec2> circleTexCoordsC;

	for (int i = 0; i < numSegments; ++i) {
		float theta = 2.0f * pi * static_cast<float>(i) / numSegments;
		float x = radiusB * cos(theta);
		float y = radiusB * sin(theta);

		// Calculate vertex positions for the circle
		glm::vec3 vertex(x, y, 0.0f);

		// Calculate normal vectors for the circle (0, 0, 1) since it's in the XY plane
		glm::vec3 normal(0.0f, 0.0f, 1.0f);

		// Calculate texture coordinates
		float u = static_cast<float>(i) / numSegments;
		float v = 0.0f; // Texture coordinate in this case can be set to 0

		// Add vertices, normals, and texture coordinates to the arrays
		circleVerticesC.push_back(vertex);
		circleNormalsC.push_back(normal);
		circleTexCoordsC.push_back(glm::vec2(u, v));
	}

	//Position, texture and normal data
	GLfloat pyramidVerts[] = {
		//Vertex Positions		//Texture coords		//Normal Coords
		//Triangle 1									//Negative Y
		-0.5f, 0.0f, 0.5f,		0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//Bottom-forward left 0
		0.5f, 0.0f, 0.5f,		1.0f, 1.0f,				0.0f, -1.0f, 0.0f,		//Bottom-forward right 1
		0.5f, 0.0f, -0.5f,		0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//Bottom-backward right 2

		//Triangle 2
		-0.5f, 0.0f, 0.5f,		0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//Bottom-forward left 0
		0.5f, 0.0f, -0.5f,		0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//Bottom-backward right 2
		-0.5f, 0.0f, -0.5f,		1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//Bottom-backward left 3

		//Triangle 3									//Positive Z
		-0.5f, 0.0f, 0.5f,		0.0f, 0.0f,				0.0f, 0.0f, 1.0f,		//Bottom-forward left 0
		0.5f, 0.0f, 0.5f,		1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		//Bottom-forward right 1
		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,				0.0f, 0.0f, 1.0f,		//Tip 4

		//Triangle 4									//Positive X
		0.5f, 0.0f, 0.5f,		1.0f, 0.0f,				1.0f, 0.0f, 0.0f,		//Bottom-forward right 1
		0.5f, 0.0f, -0.5f,		0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		//Bottom-backward right 2
		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		//Tip 4

		//Triangle 5									//Negative Z
		0.5f, 0.0f, -0.5f,		0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		//Bottom-backward right 2
		-0.5f, 0.0f, -0.5f,		1.0f, 0.0f,				0.0f, 0.0f, -1.0f,		//Bottom-backward left 3
		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//Tip 4

		//Triangle 6									//Negative X
		-0.5f, 0.0f, 0.5f,		0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		//Bottom-forward left 0
		-0.5f, 0.0f, -0.5f,		1.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		//Bottom-backward left 3
		0.0f, 1.0f, 0.0f,		1.0f, 1.0f,				-1.0f, 0.0f, 0.0f		//Tip 4
	};

	// Position and Color data
	GLfloat cubeVerts[] = {
		// Vertex Positions    //Texture coords			//Normal Coords
		//Triangle 1									//Positive Z
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Right Vertex 0 
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Right Vertex 1
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Left Vertex 3

		//Triangle 2									//Positive Z
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Right Vertex 1
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Left Vertex 2
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Left Vertex 3

		//Triangle 3									//Positive X
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		// Top Right Vertex 0
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// Bottom Right Vertex 1
		 0.5f, -0.5f, -1.0f,  0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// 4 br  right

		 //Triangle 4									//Positive X
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		// Top Right Vertex 0
		 0.5f, -0.5f, -1.0f,  0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// 4 br  right
		 0.5f,  0.5f, -1.0f,  0.0f, 1.0f,				1.0f, 0.0f, 0.0f,		//  5 tl  right

		 //Triangle 5									//Positive Y
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Right Vertex 0
		 0.5f,  0.5f, -1.0f,  0.0f, 1.0f,				0.0f, 1.0f, 0.0f,		//  5 tl  right
		-0.5f,  0.5f, -1.0f,  0.0f, 1.0f,				0.0f, 1.0f, 0.0f,		//  6 tl  top

		//Triangle 6									//Positive Y
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Right Vertex 0
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Left Vertex 3
		-0.5f,  0.5f, -1.0f,  1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		//  6 tl  top

		//Triangle 7									//Negative Z
		 0.5f, -0.5f, -1.0f,  0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		// 4 br  right
		 0.5f,  0.5f, -1.0f,  0.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  5 tl  right
		-0.5f,  0.5f, -1.0f,  1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  6 tl  top

		//Triangle 8									//Negative Z
		 0.5f, -0.5f, -1.0f,  0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		// 4 br  right
		-0.5f,  0.5f, -1.0f,  1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  6 tl  top
		-0.5f, -0.5f, -1.0f,  1.0f, 0.0f,				0.0f, 0.0f, -1.0f,		//  7 bl back


		//Triangle 9									//Negative X
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		// Bottom Left Vertex 2
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		// Top Left Vertex 3
		-0.5f,  0.5f, -1.0f,  1.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		//  6 tl  top

		//Triangle 10									//Negative X
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		// Bottom Left Vertex 2
		-0.5f,  0.5f, -1.0f,  1.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		//  6 tl  top
		-0.5f, -0.5f, -1.0f,  1.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		//  7 bl back

		//Triangle 11									//Negative Y
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Right Vertex 1
		 0.5f, -0.5f, -1.0f,  0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// 4 br  right
		-0.5f, -0.5f, -1.0f,  1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//  7 bl back

		//Triangle 12									//Negative Y
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Right Vertex 1
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Left Vertex 2
		-0.5f, -0.5f, -1.0f,  1.0f, 0.0f,				0.0f, -1.0f, 0.0f		//  7 bl back
	};

	// Position and Color data
	GLfloat boxVerts[] = {
		// Vertex Positions    //Texture coords			//Normal Coords
		//Triangle 1									//Positive Z
		 0.75f,  0.5f, 0.0f,   1.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Right Vertex 0 
		 0.75f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Right Vertex 1
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Left Vertex 3

		//Triangle 2									//Positive Z
		 0.75f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Right Vertex 1
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Left Vertex 2
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Left Vertex 3

		//Triangle 3									//Positive X
		 0.75f,  0.5f, 0.0f,   1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		// Top Right Vertex 0
		 0.75f, -0.5f, 0.0f,   1.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// Bottom Right Vertex 1
		 0.75f, -0.5f, -2.0f,  0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// 4 br  right

		 //Triangle 4									//Positive X
		 0.75f,  0.5f, 0.0f,   1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		// Top Right Vertex 0
		 0.75f, -0.5f, -2.0f,  0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// 4 br  right
		 0.75f,  0.5f, -2.0f,  0.0f, 1.0f,				1.0f, 0.0f, 0.0f,		//  5 tl  right

		 //Triangle 5									//Positive Y
		 0.75f,  0.5f, 0.0f,   1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Right Vertex 0
		 0.75f,  0.5f, -2.0f,  1.0f, 0.0f,				0.0f, 1.0f, 0.0f,		//  5 tl  right
		-0.5f,  0.5f, -2.0f,  0.0f, 0.0f,				0.0f, 1.0f, 0.0f,		//  6 tl  top

		//Triangle 6									//Positive Y
		 0.75f,  0.5f, 0.0f,   1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Right Vertex 0
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Left Vertex 3
		-0.5f,  0.5f, -2.0f,  0.0f, 0.0f,				0.0f, 1.0f, 0.0f,		//  6 tl  top

		//Triangle 7									//Negative Z
		 0.75f, -0.5f, -2.0f,  0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		// 4 br  right
		 0.75f,  0.5f, -2.0f,  0.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  5 tl  right
		-0.5f,  0.5f, -2.0f,  1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  6 tl  top

		//Triangle 8									//Negative Z
		 0.75f, -0.5f, -2.0f,  0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		// 4 br  right
		-0.5f,  0.5f, -2.0f,  1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  6 tl  top
		-0.5f, -0.5f, -2.0f,  1.0f, 0.0f,				0.0f, 0.0f, -1.0f,		//  7 bl back


		//Triangle 9									//Negative X
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		// Bottom Left Vertex 2
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		// Top Left Vertex 3
		-0.5f,  0.5f, -2.0f,  1.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		//  6 tl  top

		//Triangle 10									//Negative X
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		// Bottom Left Vertex 2
		-0.5f,  0.5f, -2.0f,  1.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		//  6 tl  top
		-0.5f, -0.5f, -2.0f,  1.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		//  7 bl back

		//Triangle 11									//Negative Y
		 0.75f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Right Vertex 1
		 0.75f, -0.5f, -2.0f,  0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// 4 br  right
		-0.5f, -0.5f, -2.0f,  1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//  7 bl back

		//Triangle 12									//Negative Y
		 0.75f, -0.5f, 0.0f,   1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Right Vertex 1
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Left Vertex 2
		-0.5f, -0.5f, -2.0f,  1.0f, 0.0f,				0.0f, -1.0f, 0.0f		//  7 bl back
	};

	GLfloat planeVerts[] = {
		// Vertex Positions    //Vertex coords			//Normal Coords
		//Triangle 1									//Positive Y
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f,				0.0f, -1.0f, 0.0f,
		0.5f, -0.5f, -0.5f,		1.0f, 0.0f,				0.0f, -1.0f, 0.0f,
		0.5f, -0.5f,  0.5f,		1.0f, 1.0f,				0.0f, -1.0f, 0.0f,

		//Triangle 1									//Positive Y
		0.5f, -0.5f,  0.5f,		1.0f, 1.0f,				0.0f, -1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,	0.0f, 1.0f,				0.0f, -1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f,				0.0f, -1.0f, 0.0f
	};

	GLfloat watchVerts[] = {
		// Vertex Positions    //Texture coords			//Normal Coords
		//Triangle 1									//Positive Z
		 0.1f,  0.02f, 0.0f,   1.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Right Vertex 0 
		 0.1f, -0.02f, 0.0f,   1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Right Vertex 1
		-0.1f,  0.02f, 0.0f,   0.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Left Vertex 3

		//Triangle 2									//Positive Z
		 0.1f, -0.02f, 0.0f,   1.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Right Vertex 1
		-0.1f, -0.02f, 0.0f,   0.0f, 0.0f,				0.0f, 0.0f, 1.0f,		// Bottom Left Vertex 2
		-0.1f,  0.02f, 0.0f,   0.0f, 1.0f,				0.0f, 0.0f, 1.0f,		// Top Left Vertex 3

		//Triangle 3									//Positive X
		 0.1f,  0.02f, 0.0f,   1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		// Top Right Vertex 0
		 0.1f, -0.02f, 0.0f,   1.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// Bottom Right Vertex 1
		 0.1f, -0.02f, -2.0f,  0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// 4 br  right

		 //Triangle 4									//Positive X
		 0.1f,  0.02f, 0.0f,   1.0f, 1.0f,				1.0f, 0.0f, 0.0f,		// Top Right Vertex 0
		 0.1f, -0.02f, -2.0f,  0.0f, 0.0f,				1.0f, 0.0f, 0.0f,		// 4 br  right
		 0.1f,  0.02f, -2.0f,  0.0f, 1.0f,				1.0f, 0.0f, 0.0f,		//  5 tl  right

		 //Triangle 5									//Positive Y
		 0.1f,  0.02f, 0.0f,   1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Right Vertex 0
		 0.1f,  0.02f, -2.0f,  1.0f, 0.0f,				0.0f, 1.0f, 0.0f,		//  5 tl  right
		-0.1f,  0.02f, -2.0f,  0.0f, 0.0f,				0.0f, 1.0f, 0.0f,		//  6 tl  top

		//Triangle 6									//Positive Y
		 0.1f,  0.02f, 0.0f,   1.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Right Vertex 0
		-0.1f,  0.02f, 0.0f,   0.0f, 1.0f,				0.0f, 1.0f, 0.0f,		// Top Left Vertex 3
		-0.1f,  0.02f, -2.0f,  0.0f, 0.0f,				0.0f, 1.0f, 0.0f,		//  6 tl  top

		//Triangle 7									//Negative Z
		 0.1f, -0.02f, -2.0f,  0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		// 4 br  right
		 0.1f,  0.02f, -2.0f,  0.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  5 tl  right
		-0.1f,  0.02f, -2.0f,  1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  6 tl  top

		//Triangle 8									//Negative Z
		 0.1f, -0.02f, -2.0f,  0.0f, 0.0f,				0.0f, 0.0f, -1.0f,		// 4 br  right
		-0.1f,  0.02f, -2.0f,  1.0f, 1.0f,				0.0f, 0.0f, -1.0f,		//  6 tl  top
		-0.1f, -0.02f, -2.0f,  1.0f, 0.0f,				0.0f, 0.0f, -1.0f,		//  7 bl back


		//Triangle 9									//Negative X
		-0.1f, -0.02f, 0.0f,   0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		// Bottom Left Vertex 2
		-0.1f,  0.02f, 0.0f,   0.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		// Top Left Vertex 3
		-0.1f,  0.02f, -2.0f,  1.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		//  6 tl  top

		//Triangle 10									//Negative X
		-0.1f, -0.02f, 0.0f,   0.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		// Bottom Left Vertex 2
		-0.1f,  0.02f, -2.0f,  1.0f, 1.0f,				-1.0f, 0.0f, 0.0f,		//  6 tl  top
		-0.1f, -0.02f, -2.0f,  1.0f, 0.0f,				-1.0f, 0.0f, 0.0f,		//  7 bl back

		//Triangle 11									//Negative Y
		 0.1f, -0.02f, 0.0f,   1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Right Vertex 1
		 0.1f, -0.02f, -2.0f,  0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// 4 br  right
		-0.1f, -0.02f, -2.0f,  1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		//  7 bl back

		//Triangle 12									//Negative Y
		 0.1f, -0.02f, 0.0f,   1.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Right Vertex 1
		-0.1, -0.02f, 0.0f,   0.0f, 0.0f,				0.0f, -1.0f, 0.0f,		// Bottom Left Vertex 2
		-0.1f, -0.02f, -2.0f,  1.0f, 0.0f,				0.0f, -1.0f, 0.0f		//  7 bl back
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerUV = 2;
	const GLuint floatsPerNormal = 3;

	mesh.nVertices[0] = sizeof(pyramidVerts) / sizeof(pyramidVerts[0]) * (floatsPerVertex + floatsPerUV);
	mesh.nVertices[1] = sizeof(cubeVerts) / sizeof(cubeVerts[0]) * (floatsPerVertex + floatsPerUV);
	mesh.nVertices[2] = sizeof(planeVerts) / sizeof(planeVerts[0]) * (floatsPerVertex + floatsPerUV);
	mesh.nVertices[3] = sizeof(boxVerts) / sizeof(boxVerts[0]) * (floatsPerVertex + floatsPerUV);
	mesh.nVertices[4] = sideVertices.size();
	mesh.nVertices[5] = circleVertices.size();
	mesh.nVertices[6] = circleVerticesB.size();
	mesh.nVertices[7] = sideVerticesB.size();
	mesh.nVertices[8] = circleNormalsC.size();
	mesh.nVertices[9] = sizeof(watchVerts) / sizeof(watchVerts[0]) * (floatsPerVertex + floatsPerUV);
	mesh.nVertices[10] = sizeof(watchVerts) / sizeof(watchVerts[0]) * (floatsPerVertex + floatsPerUV);

	//Strides between vertex coordinates is 6(x, y, z, r, g, b, a)
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV + floatsPerNormal);

#pragma region Pyramid
	//Vertex Array for Pyramid
	glGenVertexArrays(1, &mesh.vao[0]);
	glGenBuffers(1, &mesh.vbos[0]);
	glBindVertexArray(mesh.vao[0]);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); //Activate
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVerts), pyramidVerts, GL_STATIC_DRAW);

	//Create vertex attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Cube
	//Vertex Array for Cube
	glGenVertexArrays(1, &mesh.vao[1]);
	glGenBuffers(1, &mesh.vbos[1]);
	glBindVertexArray(mesh.vao[1]);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

	//Create vertex attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);

#pragma endregion

#pragma region Plane
	//Vertex Array for Plane
	glGenVertexArrays(1, &mesh.vao[2]);
	glGenBuffers(1, &mesh.vbos[2]);
	glBindVertexArray(mesh.vao[2]);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW);

	//Create vertex attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(2);

#pragma endregion

#pragma region Box
	//Vertex Array for Plane
	glGenVertexArrays(1, &mesh.vao[3]);
	glGenBuffers(1, &mesh.vbos[3]);
	glBindVertexArray(mesh.vao[3]);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[3]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(boxVerts), boxVerts, GL_STATIC_DRAW);

	//Create vertex attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(2);

#pragma endregion

#pragma region Bottle Body
	//Vertex Array for Bottle
	glGenVertexArrays(1, &mesh.vao[4]);
	glGenBuffers(1, &mesh.vbos[4]);
	glBindVertexArray(mesh.vao[4]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[4]);

	// Load vertex data into the VBO
	glBufferData(GL_ARRAY_BUFFER, sideVertices.size() * sizeof(glm::vec3), &sideVertices[0], GL_STATIC_DRAW);

	// Set up attribute pointers
	// Vertex positons
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Normals
	glGenBuffers(1, &mesh.vbos[5]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[5]);
	glBufferData(GL_ARRAY_BUFFER, sideNormals.size() * sizeof(glm::vec3), &sideNormals[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Texture
	glGenBuffers(1, &mesh.vbos[6]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[6]);
	glBufferData(GL_ARRAY_BUFFER, sideTexCoords.size() * sizeof(glm::vec2), &sideTexCoords[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

#pragma endregion

#pragma region Bottle Top and Bottom
	// Vertex Array for top circle
	glGenVertexArrays(1, &mesh.vao[5]);
	glGenBuffers(1, &mesh.vbos[7]);
	glBindVertexArray(mesh.vao[5]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[7]);

	// Load vertex data into the VBO
	glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(glm::vec3), &circleVertices[0], GL_STATIC_DRAW);

	// Set up attribute pointers
	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Normals
	glGenBuffers(1, &mesh.vbos[8]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[8]);
	glBufferData(GL_ARRAY_BUFFER, circleNormals.size() * sizeof(glm::vec3), &circleNormals[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Texture
	glGenBuffers(1, &mesh.vbos[9]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[9]);
	glBufferData(GL_ARRAY_BUFFER, circleTexCoords.size() * sizeof(glm::vec2), &circleTexCoords[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Vertex Array for bottom circle
	glGenVertexArrays(1, &mesh.vao[6]);
	glGenBuffers(1, &mesh.vbos[10]);
	glBindVertexArray(mesh.vao[6]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[10]);

	// Load vertex data into the VBO
	glBufferData(GL_ARRAY_BUFFER, circleVerticesB.size() * sizeof(glm::vec3), &circleVerticesB[0], GL_STATIC_DRAW);

	// Set up attribute pointers
	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Normals
	glGenBuffers(1, &mesh.vbos[11]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[11]);
	glBufferData(GL_ARRAY_BUFFER, circleNormalsB.size() * sizeof(glm::vec3), &circleNormalsB[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Texture
	glGenBuffers(1, &mesh.vbos[12]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[12]);
	glBufferData(GL_ARRAY_BUFFER, circleTexCoordsB.size() * sizeof(glm::vec2), &circleTexCoordsB[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

#pragma endregion

#pragma region Cap Body
	//Vertex Array for Bottle
	glGenVertexArrays(1, &mesh.vao[7]);
	glGenBuffers(1, &mesh.vbos[13]);
	glBindVertexArray(mesh.vao[7]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[13]);

	// Load vertex data into the VBO
	glBufferData(GL_ARRAY_BUFFER, sideVerticesB.size() * sizeof(glm::vec3), &sideVerticesB[0], GL_STATIC_DRAW);

	// Set up attribute pointers
	// Vertex positons
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Normals
	glGenBuffers(1, &mesh.vbos[14]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[14]);
	glBufferData(GL_ARRAY_BUFFER, sideNormalsB.size() * sizeof(glm::vec3), &sideNormalsB[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Texture
	glGenBuffers(1, &mesh.vbos[15]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[15]);
	glBufferData(GL_ARRAY_BUFFER, sideTexCoordsB.size() * sizeof(glm::vec2), &sideTexCoordsB[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

#pragma endregion

#pragma region CAP Top
	// Vertex Array for top circle
	glGenVertexArrays(1, &mesh.vao[8]);
	glGenBuffers(1, &mesh.vbos[16]);
	glBindVertexArray(mesh.vao[8]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[16]);

	// Load vertex data into the VBO
	glBufferData(GL_ARRAY_BUFFER, circleVerticesC.size() * sizeof(glm::vec3), &circleVerticesC[0], GL_STATIC_DRAW);

	// Set up attribute pointers
	// Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Normals
	glGenBuffers(1, &mesh.vbos[17]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[17]);
	glBufferData(GL_ARRAY_BUFFER, circleNormalsC.size() * sizeof(glm::vec3), &circleNormalsC[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Texture
	glGenBuffers(1, &mesh.vbos[18]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[18]);
	glBufferData(GL_ARRAY_BUFFER, circleTexCoordsC.size() * sizeof(glm::vec2), &circleTexCoordsC[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

#pragma endregion

#pragma region Watch Hands
	//Vertex Array for Plane
	glGenVertexArrays(1, &mesh.vao[9]);
	glGenBuffers(1, &mesh.vbos[19]);
	glBindVertexArray(mesh.vao[9]);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[19]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(watchVerts), watchVerts, GL_STATIC_DRAW);

	//Create vertex attribute pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float)* floatsPerVertex));
	glEnableVertexAttribArray(2);

#pragma endregion

}



void UDestroyMesh(GLMesh &mesh){
	glDeleteVertexArrays(10, mesh.vao);
	glDeleteBuffers(10, mesh.vbos);
}

bool UCreateTexture(const char* fileName, GLuint& textureId) {
	int width, height, channels;
	unsigned char* image = stbi_load(fileName, &width, &height, &channels, 0);
	if (image) {
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		//Texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//Texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture.

		return true;
	}

	// Error loading the image
	return false;
}

void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}

//Implements UCreateShaders
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId){
	//Compilation and linkage error report
	int success = 0;
	char infoLog[512];

	//Create shader program object
	programId = glCreateProgram();

	//Create vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	//Retrive source code
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	//Compile vertex shader and report errors
	glCompileShader(vertexShaderId);

	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	//Compile fragment shader and report errors
	glCompileShader(fragmentShaderId);

	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	//Attach compiled shaders to program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);		//Link shader program
	//Check for errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);		//Use shader program

	return true;
}

void UDestroyShaderProgram(GLuint programId) {
	glDeleteProgram(programId);
}