#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

GLFWwindow* window;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"


int main(void)
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1024, 768, "Benches Room", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    GLuint programID = LoadShaders(
        "TransformVertexShader.vertexshader",
        "TextureFragmentShader.fragmentshader"
    );

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    bool res = loadOBJ("bench.obj", vertices, uvs, normals);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // positions VBO (location = 0)
    GLuint posVBO;
    glGenBuffers(1, &posVBO);
    glBindBuffer(GL_ARRAY_BUFFER, posVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // uvs VBO (location = 1)
    GLuint uvVBO;
    glGenBuffers(1, &uvVBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // normals VBO (location = 2) - ensure normals vector filled by loadOBJ
    GLuint normVBO;
    glGenBuffers(1, &normVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Unbind (safe)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    auto checkErr = [](const char* where) {
        GLenum e;
        while ((e = glGetError()) != GL_NO_ERROR) {
            printf("GL error after %s: 0x%x\n", where, e);
        }
        };

    GLuint diffuseMap;
    glGenTextures(1, &diffuseMap);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);

    // parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // no mipmaps yet
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int texWidth, texHeight, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("bench_diffuse.jpg", &texWidth, &texHeight, &nrChannels, 0);
    printf("bench_diffuse.jpg: width=%d height=%d channels=%d\n", texWidth, texHeight, nrChannels);

    if (!data) {
        fprintf(stderr, "Failed to load texture (null data)\n");
        return -1;
    }

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
    checkErr("glTexImage2D");

    // optional later: glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    printf("texture_loaded\n");

    // set sampler to texture unit 0 (AFTER linking program)
    glUseProgram(programID);
    GLint texLoc = glGetUniformLocation(programID, "texture_diffuse");
    glUniform1i(texLoc, 0);




    float benchWidth = 1.55f;
    float benchDepth = 0.77f;
    float colSpacing = 0.50f;
    float rowSpacing = 0.05f;

    float stepX = benchWidth + colSpacing;   
    float stepZ = benchDepth + rowSpacing; 

    float offsetX = -(stepX * 3) * 0.5f;
    float offsetZ = -(stepZ * 5) * 0.5f;

    glm::mat4 ProjectionMatrix = glm::perspective(
        glm::radians(45.0f),        // Field of View
        4.0f / 3.0f,                // Aspect Ratio
        0.1f,                       // Near Plane
        100.0f                      // Far Plane
    );

    glm::mat4 ViewMatrix = glm::lookAt(
        glm::vec3(-4.5f, 3.0f, -4.0f),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),    // Look at point
        glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector
    );


    do {
        glUseProgram(programID);

        // set camera & projection uniforms (compute ProjectionMatrix, ViewMatrix as you already do)
        GLuint viewLoc = glGetUniformLocation(programID, "view");
        GLuint projLoc = glGetUniformLocation(programID, "projection");
        GLuint modelLoc = glGetUniformLocation(programID, "model");
        GLuint viewPosLoc = glGetUniformLocation(programID, "viewPos");
        GLuint lightPosLoc = glGetUniformLocation(programID, "lightPos");
        GLuint lightColorLoc = glGetUniformLocation(programID, "lightColor");

        // upload view/proj
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &ViewMatrix[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &ProjectionMatrix[0][0]);

        // set camera position and light info
        glUniform3f(viewPosLoc, -4.5f, 3.0f, -4.0f); // same as camera eye
        glUniform3f(lightPosLoc, 2.0f, 4.0f, 2.0f);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

        // bind texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);

        // bind VAO
        glBindVertexArray(VAO);

        // draw instances
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 6; c++) {
                glm::vec3 pos(
                    offsetX + c * stepX,  // stepX is across X
                    0.0f,
                    offsetZ + r * stepZ   // stepZ is along Z
                );
                glm::mat4 Model = translate(glm::mat4(1.0f), pos);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &Model[0][0]);

                // number of vertices:
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
            }
        }

        // cleanup
        glBindVertexArray(0);
        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    glDeleteBuffers(1, &posVBO);
    glDeleteProgram(programID);
    glfwTerminate();
    return 0;
}
