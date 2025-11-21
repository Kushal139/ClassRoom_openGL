#include "objloadermaterial.hpp"
#include <stdio.h>
#include <string.h>
#include <unordered_map>

struct TempFaceIndex {
    unsigned v, t, n;
};

bool loadOBJWithMaterials(const char* path, std::vector<MaterialMesh>& meshes)
{
    FILE* file = fopen(path, "r");
    if (!file) return false;

    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::unordered_map<std::string, MaterialMesh> meshMap;
    std::string currentMaterial = "default";

    char line[256];
    while (fscanf(file, "%s", line) != EOF) {
        if (strcmp(line, "v") == 0) {
            glm::vec3 v; fscanf(file, "%f %f %f", &v.x, &v.y, &v.z);
            temp_vertices.push_back(v);
        }
        else if (strcmp(line, "vt") == 0) {
            glm::vec2 uv; fscanf(file, "%f %f", &uv.x, &uv.y);
            temp_uvs.push_back(uv);
        }
        else if (strcmp(line, "vn") == 0) {
            glm::vec3 n; fscanf(file, "%f %f %f", &n.x, &n.y, &n.z);
            temp_normals.push_back(n);
        }
        else if (strcmp(line, "usemtl") == 0) {
            char matName[128];
            fscanf(file, "%s", matName);
            currentMaterial = matName;
            if (meshMap.find(currentMaterial) == meshMap.end()) {
                meshMap[currentMaterial] = MaterialMesh();
                meshMap[currentMaterial].materialName = currentMaterial;
            }
        }
        else if (strcmp(line, "f") == 0) {
            TempFaceIndex f[3];
            fscanf(file,
                "%u/%u/%u %u/%u/%u %u/%u/%u",
                &f[0].v, &f[0].t, &f[0].n,
                &f[1].v, &f[1].t, &f[1].n,
                &f[2].v, &f[2].t, &f[2].n);

            MaterialMesh& mesh = meshMap[currentMaterial];

            for (int i = 0; i < 3; i++) {
                mesh.vertices.push_back(temp_vertices[f[i].v - 1]);
                mesh.uvs.push_back(temp_uvs[f[i].t - 1]);
                mesh.normals.push_back(temp_normals[f[i].n - 1]);
            }
        }
        else {
            fgets(line, sizeof(line), file);
        }
    }

    fclose(file);

    for (auto& kv : meshMap) meshes.push_back(kv.second);
    return true;
}
