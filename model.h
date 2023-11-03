#ifndef MODEL_H
#define MODEL_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "mesh.h"

#include <vector>
#include <string>

unsigned int TextureFromFile(const char* path, const std::string& directory, bool clamp = false);

class Model {
public:
	Model(const char* path) {
		loadModel(path);
	}
	void Draw(Shader& shader);

	std::vector<Mesh> meshes;
	std::vector<Texture> texturesLoaded;
private:
	// model data
	std::string directory;

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

#endif
