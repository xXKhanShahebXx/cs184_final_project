#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Cloth.h"

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();
    
    bool initialize();
    void render(const Cloth& cloth);
    void processInput(GLFWwindow* window);
    bool shouldClose() const;
    
    GLFWwindow* getWindow() const { return window; }
    
private:
    GLFWwindow* window;
    int width, height;
    
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    
    void setupShaders();
    void setupBuffers();
    void drawCloth(const Cloth& cloth);
    void drawSurface();
}; 