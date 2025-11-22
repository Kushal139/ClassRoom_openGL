// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

struct GLMesh {
    GLuint vao, vertexbuffer, uvbuffer, normalbuffer, elementbuffer;
	std::vector<unsigned short> indices;
    GLuint textureID;
	glm::vec3 metarialColor;
    bool useTexture;
    int vertexCount;
};

std::vector<GLMesh> GLMeshes;

int main( void )
{
	// Initialize GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Classroom", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 
	glEnable(GL_CULL_FACE);

	GLuint phongProgram = LoadShaders( "Phong.vertexshader", "Phong.fragmentshader" );
	GLuint gouraudProgram = LoadShaders( "Gouraud.vertexshader", "Gouraud.fragmentshader" );

	bool usePhong = false;
	// switch in runtime
	GLuint programID = usePhong ? phongProgram : gouraudProgram;	

	const int NUM_LIGHTS = 9;
	std::vector<glm::vec3> lightPositions;
	float roomX = 9.0f;
	float roomY = 3.1f;
	float roomZ = 7.0f;

	float stepX = roomX / 3.0f;
	float stepZ = roomZ / 3.0f;    

	for(int iy = 1; iy <= 3; iy++){
		for(int ix = 1; ix <= 3; ix++){
			float x = stepX * ix - stepX/2.0f;
			float y = 0.0f;
			float z = stepZ * iy - stepZ/2.0f;;

			lightPositions.push_back(glm::vec3(x, y, z));
			std::cout << "std::cout << \"light at ("
					<< x << ", " << y << ", " << z << ")\\n\";\n\n";
		}
	}

	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
	GLuint MaterialColorID = glGetUniformLocation(programID, "materialColor");


	// ---------- Load OBJ with materials ----------
    std::vector<MaterialMesh> materialMeshes;
    loadOBJWithMaterials("room.obj", materialMeshes);

    std::cout << "Loaded " << materialMeshes.size() << " material meshes\n";

	glm::vec3 minV(FLT_MAX), maxV(-FLT_MAX);
	for (auto &m : materialMeshes) {
		for (auto &v : m.vertices) {
			minV.x = std::min(minV.x, v.x);
			minV.y = std::min(minV.y, v.y);
			minV.z = std::min(minV.z, v.z);

			maxV.x = std::max(maxV.x, v.x);
			maxV.y = std::max(maxV.y, v.y);
			maxV.z = std::max(maxV.z, v.z);
		}
	}

	glm::vec3 modelCenter = (minV + maxV) * 0.5f;
	glm::vec3 modelSize   = maxV - minV;

	std::cout << "Model center: " 
			<< modelCenter.x << ", " 
			<< modelCenter.y << ", " 
			<< modelCenter.z << std::endl;

	std::cout << "Model size: " 
			<< modelSize.x << ", "
			<< modelSize.y << ", "
			<< modelSize.z << std::endl;

	std::cout << "Bounding box min: " 
			<< minV.x << ", " 
			<< minV.y << ", " 
			<< minV.z << std::endl;
	std::cout << "Bounding box max: " 
			<< maxV.x << ", "
			<< maxV.y << ", "
			<< maxV.z << std::endl;


	// from material meshes to GLMeshes
    for (auto& m : materialMeshes)
    {
        GLMesh glmesh;
		glmesh.vertexCount = m.vertices.size();
        glmesh.useTexture = false;
        glmesh.textureID = 0;

        if (m.materialName == "wood") {
            glmesh.textureID = loadBMP_custom("bench_wood.bmp");
            glmesh.useTexture = true;
            std::cout << "Wood mesh uses texture bench_texture.bmp\n";
        }
        else if (m.materialName == "board") {
			glmesh.useTexture = false;
			glmesh.metarialColor = glm::vec3(0.0f, 0.55f, 0.29f); // dark green
            std::cout << "green board\n";
        }
		else if (m.materialName == "projector"){
			glmesh.useTexture = false;
			glmesh.metarialColor = glm::vec3(0.8f, 0.8f, 0.8f); // light gray
			std::cout << "projector\n";
		}
		else if(m.materialName == "podium"){
			glmesh.useTexture = false;
			glmesh.metarialColor = glm::vec3(0.88f, 0.63f, 0.27f); // 
			std::cout << "podium\n";
		}
		else if (m.materialName == "wall") {
			glmesh.useTexture = false;
			glmesh.metarialColor = glm::vec3(1.0f, .99f, .81f); // yellowish
		}
		else if (m.materialName == "metal"){
			glmesh.useTexture = false;
			glmesh.metarialColor = glm::vec3(0.8f, 0.8f, 0.8f); // light gray
		}
		else if (m.materialName == "floor") {
			glmesh.useTexture = true;
			glmesh.textureID = loadBMP_custom("floor_texture.bmp");
		}

		MaterialMesh indexedMesh = m;
		/*indexedMesh.materialName = m.materialName;*/

		std::vector<unsigned short> indices;
		//indexVBO(m.vertices, m.uvs, m.normals, indices, indexedMesh.vertices, indexedMesh.uvs, indexedMesh.normals);
		glmesh.indices = indices;
        glGenVertexArrays(1, &glmesh.vao);
        glBindVertexArray(glmesh.vao);

		glGenBuffers(1, &glmesh.vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, glmesh.vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, indexedMesh.vertices.size()*sizeof(glm::vec3), &indexedMesh.vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &glmesh.uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, glmesh.uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, indexedMesh.uvs.size()*sizeof(glm::vec2), &indexedMesh.uvs[0], GL_STATIC_DRAW);

        glGenBuffers(1, &glmesh.normalbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, glmesh.normalbuffer);
        glBufferData(GL_ARRAY_BUFFER, indexedMesh.normals.size()*sizeof(glm::vec3), &indexedMesh.normals[0], GL_STATIC_DRAW);

		/*glGenBuffers(1, &glmesh.elementbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glmesh.elementbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);*/
        GLMeshes.push_back(glmesh);
    }

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	do{
		double currentTime = glfwGetTime();
		nbFrames++;
			if(glfwGetKey(window, GLFW_KEY_SPACE ) == GLFW_PRESS){
			usePhong = !usePhong;
			while(glfwGetKey(window, GLFW_KEY_SPACE ) == GLFW_PRESS){
				glfwPollEvents();
			}
		}
		programID = usePhong ? phongProgram : gouraudProgram;
		if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0/double(nbFrames));
			printf("%s \n", usePhong ? "Phong" : "Gouraud");
			nbFrames = 0;
			lastTime += 1.0;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(programID);
		GLint lightPosLoc = glGetUniformLocation(programID, "LightPosition_worldspace");
		glUniform3fv(lightPosLoc, NUM_LIGHTS, &lightPositions[0].x);

		GLuint MatrixID = glGetUniformLocation(programID, "MVP");
		GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
		GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
		GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
		GLuint MaterialColorID = glGetUniformLocation(programID, "materialColor");

		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

        for (auto& m : GLMeshes) {

            glBindVertexArray(m.vao);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, m.vertexbuffer);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, m.uvbuffer);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

            glEnableVertexAttribArray(2);
            glBindBuffer(GL_ARRAY_BUFFER, m.normalbuffer);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

			// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.elementbuffer);

            if (m.useTexture) {
				glUniform1i(glGetUniformLocation(programID,"useTexture"), 1);
				glUniform3f(MaterialColorID, 1,1,1);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, m.textureID);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glUniform1i(TextureID, 0);
			} else {
				glUniform1i(glGetUniformLocation(programID,"useTexture"), 0);
				glUniform3f(MaterialColorID, m.metarialColor.x, m.metarialColor.y, m.metarialColor.z);
			}
            //glDrawElements(GL_TRIANGLES, m.indices.size(), GL_UNSIGNED_SHORT, (void*)0);
			glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
        }
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );
	return 0;
}

