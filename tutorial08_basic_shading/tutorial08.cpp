// ====== Includes ======
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Tutorial common headers
#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/texture.hpp>
#include <common/objloadermaterial.hpp>


struct RenderMesh {
    std::string material;
    GLuint vao, vbo_vertices, vbo_uvs, vbo_normals;
    int vertexCount;
};
std::vector<RenderMesh> renderMeshes;

int main()
{
    // ====== GLFW Init ======
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1024, 768, "classroom", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // ====== GLEW Init ======
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwPollEvents();
    glfwSetCursorPos(window, 1024 / 2, 768 / 2);

    // OpenGL states
    glClearColor(0.3f, 0.4f, 0.6f, 0.0f); // background
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // glEnable(GL_CULL_FACE);

    // ====== Load shaders ======
    GLuint programID = LoadShaders(
        "StandardShading.vertexshader",
        "StandardShading.fragmentshader"
    );

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
    GLuint MaterialColorID = glGetUniformLocation(programID, "materialColor");
    GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");
    GLuint UseTextureID = glGetUniformLocation(programID, "useTexture");

    glUseProgram(programID);
    glUniform1i(TextureID, 0); // sampler uses texture unit 0

    std::vector<MaterialMesh> materialMeshes;
    if (!loadOBJWithMaterials("bench.obj", materialMeshes)) {
        std::cout << "OBJ load failed\n";
        return -1;
    }

    std::cout << "Loaded materials: " << materialMeshes.size() << "\n";

    for (auto& m : materialMeshes) {
        RenderMesh rm;
        rm.material = m.materialName;
        rm.vertexCount = m.vertices.size();

        glGenVertexArrays(1, &rm.vao);
        glBindVertexArray(rm.vao);

        // ---- positions ----
        glGenBuffers(1, &rm.vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, rm.vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, m.vertices.size() * sizeof(glm::vec3),
            m.vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // ---- uvs ----
        glGenBuffers(1, &rm.vbo_uvs);
        glBindBuffer(GL_ARRAY_BUFFER, rm.vbo_uvs);
        glBufferData(GL_ARRAY_BUFFER, m.uvs.size() * sizeof(glm::vec2),
            m.uvs.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // ---- normals ----
        glGenBuffers(1, &rm.vbo_normals);
        glBindBuffer(GL_ARRAY_BUFFER, rm.vbo_normals);
        glBufferData(GL_ARRAY_BUFFER, m.normals.size() * sizeof(glm::vec3),
            m.normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glBindVertexArray(0);

        renderMeshes.push_back(rm);
        std::cout << "Mesh: " << m.materialName
            << " vertices: " << rm.vertexCount << "\n";
    };
    GLuint woodTex = loadBMP_custom("bench_texture1.bmp");

    // ====== Main loop ======
    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compute matrices from user inputs
        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();
        glm::mat4 ModelMatrix = glm::mat4(1.0f);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        glUseProgram(programID);

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

        glm::vec3 lightPos(4.0f, 4.0f, 4.0f);
        glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

        // draw all meshes
        for (auto &rm : renderMeshes) {
            glBindVertexArray(rm.vao);

            if (rm.material == "wood") {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, woodTex);
                glUniform1i(glGetUniformLocation(programID, "useTexture"), true);
            }
            else {
                glBindTexture(GL_TEXTURE_2D, 0);
                glUniform1i(glGetUniformLocation(programID, "useTexture"), false);

                // if (rm.material == "metal")
                //     glUniform3f(glGetUniformLocation(programID, "materialColor"),
                //                 0.4, 0.4, 0.4);
                glUniform3f(glGetUniformLocation(programID, "materialColor"),
                                1.0, 0.0, 1.0); // debug pink
            }

            glDrawArrays(GL_TRIANGLES, 0, rm.vertexCount);
        }


        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    glfwTerminate();
    return 0;
}
