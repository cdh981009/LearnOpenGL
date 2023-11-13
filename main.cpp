#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "stb_image.h"
#include "camera.h"
#include "model.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

using std::vector;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lastX = 400., lastY = 300.;
bool firstMouse = true;

bool debug = false;

Camera camera(glm::vec3(0.0f, 1.0f, 3.0f));

void renderScene(Shader& shader);
void renderCube();
void renderPlane();
void renderWall();

// texture loading
unsigned int cubeTexture, floorTexture;
unsigned int wallTexture, wallNormal;
unsigned int brickDiffuse, brickNormal, brickHeight;
unsigned int woodDiffuse, toyBoxNormal, toyBoxHeight;

Model* ourModel = nullptr;

int main(void) {
    // initializing window
    // -------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell OpenGL to capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // register callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // enable z buffer
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //stbi_set_flip_vertically_on_load(true);
    
    cubeTexture = TextureFromFile("container.jpg", "./resources");
    floorTexture = TextureFromFile("metal.png", "./resources");
    wallTexture = TextureFromFile("brickwall.jpg", "./resources");
    wallNormal = TextureFromFile("brickwall_normal.jpg", "./resources");

    brickDiffuse = TextureFromFile("bricks2.jpg", "./resources");
    brickNormal  = TextureFromFile("bricks2_normal.jpg", "./resources");
    brickHeight  = TextureFromFile("bricks2_disp.jpg", "./resources");

    woodDiffuse  = TextureFromFile("wood.png", "./resources");
    toyBoxNormal = TextureFromFile("toy_box_normal.png", "./resources");
    toyBoxHeight = TextureFromFile("toy_box_disp.png", "./resources");

    // shader loading
    Shader shadowShader("./shaders/parallax_map.vs", "./shaders/parallax_map.fs");
    Shader lightCubeShader("./shaders/light_cube.vs", "./shaders/light_cube.fs");
    Shader depthCubeShader("./shaders/omnidirectional_shadow_map.vs",
                           "./shaders/omnidirectional_shadow_map.fs",
                           "./shaders/omnidirectional_shadow_map.gs");

    ourModel = new Model("./resources/backpack/backpack.obj");

    // frame buffer for shadow mapping
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_DEPTH_COMPONENT,
            SHADOW_WIDTH,
            SHADOW_HEIGHT,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            NULL
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shadowShader.use();
    shadowShader.setInt("texture_diffuse1", 0);
    shadowShader.setInt("texture_normal1", 2);
    shadowShader.setInt("texture_height1", 3);
    shadowShader.setInt("depthMap", 1);

    glm::vec3 lightPosition(0.0, 5.0, -4.0);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // logic
        lightPosition.x = 2 * sin(2.0 * currentFrame);
        lightPosition.y = 4 + 2 * cos(2.0 * currentFrame);
        lightPosition.z = -2 + cos(1.0 * currentFrame);
      
        // rendering
        // ---------

        // 1. render to depth cube map
        // ---------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // configure shader and matrices
        float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
        float near = 1.0f;
        float far = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj*
            glm::lookAt(lightPosition, lightPosition + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj*
            glm::lookAt(lightPosition, lightPosition + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj*
            glm::lookAt(lightPosition, lightPosition + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj*
            glm::lookAt(lightPosition, lightPosition + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
        shadowTransforms.push_back(shadowProj*
            glm::lookAt(lightPosition, lightPosition + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj*
            glm::lookAt(lightPosition, lightPosition + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));
        depthCubeShader.use();
        depthCubeShader.setFloat("farPlane", far);
        for (int i = 0; i < 6; ++i)
            depthCubeShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        depthCubeShader.setVec3("lightPos", lightPosition);

        renderScene(depthCubeShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. render scene as normal with shadow mapping
        // ---------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        shadowShader.use();
        shadowShader.setMat4("view", view);
        shadowShader.setMat4("projection", projection);
        lightCubeShader.use();
        lightCubeShader.setMat4("view", view);
        lightCubeShader.setMat4("projection", projection);

        shadowShader.use();
        shadowShader.setVec3("lightPos", lightPosition);
        shadowShader.setFloat("farPlane", far);
        shadowShader.setVec3("lightPos", lightPosition);
        shadowShader.setVec3("viewPos", camera.Position);
        shadowShader.setBool("useNormalMap", false);
        shadowShader.setBool("useHeightMap", false);
        shadowShader.setBool("debug", debug);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

        renderScene(shadowShader);

        lightCubeShader.use();
        auto model = glm::mat4(1.0);
        model = glm::translate(model, lightPosition);
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        lightCubeShader.setMat4("model", model);
        renderCube();

        // unbind
        glBindVertexArray(0);

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void renderScene(Shader& shader) {
    shader.setInt("texture_diffuse1", 0);
    shader.setInt("texture_normal1", 2);

    glm::mat4 model;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);
    model = glm::mat4(1.0f);
    shader.setMat4("model", model);
    renderPlane();

    // ---- wall ----
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, wallNormal);
    
    shader.setBool("useNormalMap", true);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 3.0f, -5.0f));
    
    shader.setMat4("model", model);
    renderWall();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.2f, 3.0f, -5.0f));
    model = glm::rotate(model, (float)glm::radians(25.0f * sin(glfwGetTime())), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, (float)glm::radians(25.0f * cos(glfwGetTime())), glm::vec3(1.0f, 0.0f, 0.0f));

    shader.setMat4("model", model);
    renderWall();

    shader.setBool("useNormalMap", false);

    // ---- brick & toy box ----
    shader.setBool("useHeightMap", true);
    shader.setBool("useNormalMap", true);
    shader.setFloat("heightScale", 0.1f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brickDiffuse);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brickNormal);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, brickHeight);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.2f, 3.0f, -5.0f));

    shader.setMat4("model", model);
    renderWall();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-4.4f, 3.0f, -5.0f));
    model = glm::rotate(model, (float)glm::radians(25.0f * sin(glfwGetTime())), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, (float)glm::radians(25.0f * cos(glfwGetTime())), glm::vec3(1.0f, 0.0f, 0.0f));

    shader.setMat4("model", model);
    renderWall();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodDiffuse);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, toyBoxNormal);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, toyBoxHeight);

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.2f, 5.2f, -5.0f));
    model = glm::rotate(model, (float)glm::radians(25.0f * sin(glfwGetTime())), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, (float)glm::radians(25.0f * cos(glfwGetTime())), glm::vec3(1.0f, 0.0f, 0.0f));

    shader.setMat4("model", model);
    renderWall();

    shader.setBool("useHeightMap", false);
    shader.setBool("useNormalMap", false);

    // ---- cubes ----
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cubeTexture);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 4.0f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 2.0f, 0.0f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 2.5f, 2.0f));
    model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
    model = glm::rotate(model, glm::radians(60.0f), glm::vec3(1.0f, 2.0f, 1.5f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(3.0f, 0.5f, 5.0f));
    model = glm::scale(model, glm::vec3(1.0f, 2.0f, 1.0f));
    model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 3.f, 2.0f));
    model = glm::rotate(model, glm::radians(15.0f * (float) glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("model", model);
    shader.setBool("useNormalMap", true);
    ourModel->Draw(shader);
    shader.setBool("useNormalMap", false);
}

void renderWall() {
    static unsigned int wallVAO = 0, wallVBO = 0;

    if (wallVAO == 0) {
        // hande calculation of TBN matrix
        // edge1 = pos1 - pos3 = (u1 - u3) * T + (v1 - v3) * B
        // edge2 = pos1 - pos2 = (u1 - u2) * T + (v1 - v2) * B
        // rewrite in matrix multiplication
        // | e1 | = | du1 dv1 | * | T |
        // | e2 |   | du2 dv2 |   | B |
        //  =>
        // | T | = | du1 dv1 | ^ -1  * | e1 |
        // | B |   | du2 dv2 |         | e2 |
        //       = 1 /                     * | dv2  -dv1 | * | e1 |
        //         (du1 * dv2 - dv1 * du2)   | -du2  du1 | * | e2 |
        glm::vec3 pos1(-1.0, -1.0, 0.0);
        glm::vec3 pos2(1.0, -1.0, 0.0);
        glm::vec3 pos3(1.0, 1.0, 0.0);
        glm::vec3 pos4(-1.0, 1.0, 0.0);
        glm::vec2 uv1(0.0, 0.0);
        glm::vec2 uv2(1.0, 0.0);
        glm::vec2 uv3(1.0, 1.0);
        glm::vec2 uv4(0.0, 1.0);

        glm::vec3 ed1 = pos1 - pos3;
        glm::vec3 ed2 = pos1 - pos2;
        glm::vec2 deltaUV1 = uv1 - uv3;
        glm::vec2 deltaUV2 = uv1 - uv2;
        float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

        glm::vec3 T, B, N(0.0, 0.0, 1.0);
        T.x = f * (deltaUV2.y * ed1.x - deltaUV1.y * ed2.x);
        T.y = f * (deltaUV2.y * ed1.y - deltaUV1.y * ed2.y);
        T.z = f * (deltaUV2.y * ed1.z - deltaUV1.y * ed2.z);

        const static float wallVertices[] = {
            // positions         // normal         // texture  // T           
            -1.0f, -1.0f, 0.f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, T.x, T.y, T.z,
             1.0f, -1.0f, 0.f,  0.0f,  0.0f, 1.0f, 1.0f, 0.0f, T.x, T.y, T.z,
             1.0f,  1.0f, 0.f,  0.0f,  0.0f, 1.0f, 1.0f, 1.0f, T.x, T.y, T.z,
             1.0f,  1.0f, 0.f,  0.0f,  0.0f, 1.0f, 1.0f, 1.0f, T.x, T.y, T.z,
            -1.0f,  1.0f, 0.f,  0.0f,  0.0f, 1.0f, 0.0f, 1.0f, T.x, T.y, T.z,
            -1.0f, -1.0f, 0.f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f, T.x, T.y, T.z,
        };

        glGenVertexArrays(1, &wallVAO);
        glGenBuffers(1, &wallVBO);
        glBindVertexArray(wallVAO);
        glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), &wallVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glBindVertexArray(0);
    }


    glBindVertexArray(wallVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void renderCube() {
    static unsigned int cubeVAO = 0, cubeVBO = 0;
    const static float cubeVertices[] = {
        // positions         // normal           // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
    };

    if (cubeVAO == 0) {
        // cube VAO
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void renderPlane() {
    static  unsigned int planeVAO = 0, planeVBO = 0;

    static const float planeVertices[] = {
        // positions          // normal      // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         15.0f, -0.51f, 15.0f, 0.0, 1.0, 0.0, 2.0f, 0.0f,
        -15.0f, -0.51f, -5.0f, 0.0, 1.0, 0.0, 0.0f, 2.0f,
        -15.0f, -0.51f, 15.0f, 0.0, 1.0, 0.0, 0.0f, 0.0f,

         15.0f, -0.51f, 15.0f, 0.0, 1.0, 0.0, 2.0f, 0.0f,
         15.0f, -0.51f, -5.0f, 0.0, 1.0, 0.0, 2.0f, 2.0f,
        -15.0f, -0.51f, -5.0f, 0.0, 1.0, 0.0, 0.0f, 2.0f,
    };

    if (planeVAO == 0) {
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    debug = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
