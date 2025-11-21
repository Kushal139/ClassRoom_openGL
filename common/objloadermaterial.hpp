#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

struct MaterialMesh {
    std::string materialName;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
};

bool loadOBJWithMaterials(
    const char* path,
    std::vector<MaterialMesh>& meshes
);
