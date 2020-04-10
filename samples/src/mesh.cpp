#include "mesh.h"

Scene* LoadScene(const char* path, glm::vec3& min, glm::vec3& max)
{
	Scene* result = new Scene();

	Assimp::Importer meshImporter;
	const aiScene* aiScene = meshImporter.ReadFile(path, aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes);

	float minX = INFINITY;
	float minY = INFINITY;
	float minZ = INFINITY;
	float maxX = -INFINITY;
	float maxY = -INFINITY;
	float maxZ = -INFINITY;

	for (int i = 0; i < aiScene->mNumMeshes; i++)
	{
		aiMesh* aiMesh = aiScene->mMeshes[i];
		Mesh* mesh = new Mesh();
		mesh->name = aiMesh->mName.C_Str();

		mesh->indices.resize(aiMesh->mNumFaces * 3);
		for (size_t f = 0; f < aiMesh->mNumFaces; ++f)
		{
			for (size_t i = 0; i < 3; ++i)
			{
				mesh->indices[f * 3 + i] = aiMesh->mFaces[f].mIndices[i];
			}
		}

		for (size_t j = 0; j < aiMesh->mNumVertices; ++j)
		{
			Vertex vertex;

			vertex.position = glm::vec3(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z);
			vertex.position = glm::eulerAngleX(glm::radians(-90.0f)) * glm::vec4(vertex.position, 1.0f);
			vertex.position = glm::eulerAngleY(glm::radians(-90.0f)) * glm::vec4(vertex.position, 1.0f);

			vertex.normal = glm::vec3(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z);
			vertex.uv = glm::vec2(aiMesh->mTextureCoords[0][j].x, 1.0f - aiMesh->mTextureCoords[0][j].y);

			minX = Math::Min(vertex.position.x, minX);
			minY = Math::Min(vertex.position.y, minY);
			minZ = Math::Min(vertex.position.z, minZ);

			maxX = Math::Max(vertex.position.x, maxX);
			maxY = Math::Max(vertex.position.y, maxY);
			maxZ = Math::Max(vertex.position.z, maxZ);

			mesh->vertices.push_back(vertex);
		}

		mesh->CreateGPUResources();

		result->meshes.push_back(mesh);
	}

	min = glm::vec3(minX, minY, minZ);
	max = glm::vec3(maxX, maxY, maxZ);

	return result;
}

void DestroyScene(Scene* scene)
{
	for (auto mesh : scene->meshes)
	{
		delete mesh;
	}

	delete scene;
}