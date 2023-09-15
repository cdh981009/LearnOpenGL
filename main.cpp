#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    Shader ourShader("./shaders/shader.vs", "./shaders/shader.fs");

    float vertices[] = {
        // positions		 // colors
         0.5f, -0.5f, 0.0f,	 1.0f, 0.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,	 0.0f, 1.0f, 0.0f, // bottom left
         0.0f,  0.5f, 0.0f,	 0.0f, 0.0f, 1.0f, // top
    };
    //unsigned int indices[] = { // note that we start from 0!
    //	0, 1, 3, // first triangle
    //	1, 2, 3, // second triangle
    //};
    
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    /*unsigned int EBO;
    glGenBuffers(1, &EBO);*/
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    // :: Initialization code ::
    // 1. bind Vertex Array Object
    glBindVertexArray(VAO);
    // 2. copy our vertices array in a buffer for OpenGL to use	
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 3. copy our indices array in an element buffer for OpenGL to use
    /*glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);*/
    // 4. then set the vertex attributes pointers
    // this tells OpenGL how our vertices array is structured
    // so the vertex shader can work as intended
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // input
        processInput(window);

        // rendering commands here
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //float timeValue = glfwGetTime();
        //float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
        //// note that finding uniform location does not require to use the shader program first
        //// but updating (calling glUniform4f in this case) does require it
        //int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");

        // use our shader program when we want to redner an object
        ourShader.use();
        float offset = sin(2.0 * glfwGetTime()) / 2.0;
        ourShader.setFloat("offset", offset);
        //glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
        // bind VAO which indirectly binds VBO and vertex attributes
        glBindVertexArray(VAO);
        //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0); // unbind

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}