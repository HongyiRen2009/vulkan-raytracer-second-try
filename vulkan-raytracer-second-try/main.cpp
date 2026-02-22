#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "VulkanApp.h"
using namespace my_vulkan;
int windowWidth = 800;
int windowHeight = 600;
GLFWwindow* window;
VulkanApp vulkanApp;
void initWindow() {
	glfwInit();
	GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
	windowWidth = videoMode->width;
	windowHeight = videoMode->height;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);
}
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}
void render() {
	
}
static void OnWindowResized(GLFWwindow* window, int width, int height) {
	int _width = 0, _height = 0;
	glfwGetFramebufferSize(window, &_width, &_height);
	while (_width == 0 || _height == 0) {
		glfwGetFramebufferSize(window, &_width, &_height);
		glfwWaitEvents();
	}

	vulkanApp.OnWindowResized(_width, _height);
}
int main() {
	initWindow();
	vulkanApp.Init("Vulkan App", window);
	if (!glfwVulkanSupported()) {
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, OnWindowResized);
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		vulkanApp.Render();
		processInput(window);
	}
	vulkanApp.Cleanup();
	glfwTerminate();
	return 0;
}
