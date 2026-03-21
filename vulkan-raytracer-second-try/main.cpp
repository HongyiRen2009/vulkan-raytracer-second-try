#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "VulkanApp.h"
#include "ChunkManager.h"
#include <string>
using namespace my_vulkan;
int windowWidth = 800;
int windowHeight = 600;
GLFWwindow* window;
VulkanApp vulkanApp;
Camera* camera;
ChunkManager chunkManager;
unsigned int seed = 42;
ivec2 lastCameraChunkPos = ivec2(0);
vec2 lastCameraLoadPos = vec2(0);

// FPS Counter variables
double lastFrameTime = 0.0;
double fpsUpdateInterval = 0.5; // Update FPS display every 0.5 seconds
int frameCount = 0;
double averageFPS = 0.0;

static void OnWindowResized(GLFWwindow* window, int width, int height) {
	int _width = 0, _height = 0;
	glfwGetFramebufferSize(window, &_width, &_height);
	while (_width == 0 || _height == 0) {
		glfwGetFramebufferSize(window, &_width, &_height);
		glfwWaitEvents();
	}
	camera->updateAspectRatio(static_cast<float>(_width) / static_cast<float>(_height));
	vulkanApp.OnWindowResized(_width, _height);
}
void initWindow() {
	glfwInit();
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
	windowWidth = videoMode->width;
	windowHeight = videoMode->height;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, OnWindowResized);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	lastFrameTime = glfwGetTime();
}
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	camera->update();
}
void updateFPSCounter() {
	double currentTime = glfwGetTime();
	double deltaTime = currentTime - lastFrameTime;
	lastFrameTime = currentTime;

	frameCount++;
	double elapsedTime = currentTime - (lastFrameTime - deltaTime * frameCount);

	if (deltaTime > 0.0) {
		double instantFPS = 1.0 / deltaTime;
		averageFPS = averageFPS * 0.9 + instantFPS * 0.1; // Smooth average
	}

	if (elapsedTime >= fpsUpdateInterval || frameCount == 1) {
		std::string title = "Vulkan - FPS: " + std::to_string(static_cast<int>(averageFPS));
		glfwSetWindowTitle(window, title.c_str());
		frameCount = 0;
		lastFrameTime = currentTime;
	}
}
void checkForChunkUpdates() {
	ivec2 cameraChunkPos = ivec2(floor(camera->position.x / Chunk::CHUNK_SIZE), floor(camera->position.z / Chunk::CHUNK_SIZE));
	vec2 cameraLoadPos = vec2(camera->position.x, camera->position.z);
	if (cameraChunkPos != lastCameraChunkPos && distance(cameraLoadPos, lastCameraLoadPos) > Chunk::CHUNK_SIZE * 1.5) {
		chunkManager.requestChunkGeneration(ivec3(cameraChunkPos.x, 0, cameraChunkPos.y));
		lastCameraChunkPos = cameraChunkPos;
		lastCameraLoadPos = cameraLoadPos;
	}
}
void updateChunkSSBO() {
	vulkanApp.InvalidateChunksInGPU(chunkManager.getInvalidChunks());
	vulkanApp.UpdateChunkSSBO(&chunkManager);
	chunkManager.isGeneratingChunks = false;
}

int main() {
	VulkanPhysicalDevices::useIntegratedGPU = true;
	initWindow();
	camera = new Camera(window);
	camera->updateAspectRatio(static_cast<float>(windowWidth) / static_cast<float>(windowHeight));
	vulkanApp.Init("Vulkan App", window, camera);
	Chunk::populateBlockDataArray();
	chunkManager.init(seed);
	vector<uint32_t> _;
	chunkManager.requestChunkGeneration(ivec3(0),true);
	vulkanApp.UpdateChunkSSBO(&chunkManager);
	if (!glfwVulkanSupported()) {
		return -1;
	}
	while (!glfwWindowShouldClose(window)) {
		if (chunkManager.isGeneratingChunks&&chunkManager.isFinishedGeneratingChunks) {
			updateChunkSSBO();
		}
		checkForChunkUpdates();
		glfwPollEvents();
		vulkanApp.Render();
		processInput(window);
		updateFPSCounter();
	}
	vulkanApp.Cleanup();
	glfwTerminate();
	return 0;
}