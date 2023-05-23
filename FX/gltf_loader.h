#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_gltf.h>
enum AlertSeverity { asWARNING, asERROR };
typedef void(*alertFunc)(AlertSeverity as, const char* msg, void* ptr);
struct GLTFVertex {
	glm::vec3	pos;
	glm::vec3	norm;
	glm::vec2	uv;
	glm::vec3	color;
	glm::vec4	jointIndices;
	glm::vec4	jointWeights;
};
struct GLTFPrimitive {
	uint32_t	indexStart;
	uint32_t	indexCount;
	uint32_t	material;
};
struct GLTFImage {
	std::string filename;
	int32_t width;
	int32_t height;

	std::vector<uint8_t> pixels;

};
struct GLTFMaterial {
	glm::vec4 baseColorFactor = glm::vec4(1.f);
	float metallic{ 0.f };
	float roughness{ 0.f };
	uint32_t baseColorTextureIndex;
};


class GLTFLoader {



	struct Node {
		Node* parent;
		uint32_t			index;
		std::vector<Node*>	children;

		glm::vec3			translation{ 0.f };
		glm::vec3			scale{ 1.f };
		glm::quat			rotation{};
		uint32_t			skin{ UINT32_MAX };
		glm::mat4			matrix;
		std::string			name;
	};
	struct Skin {
		std::string name;
		Node* skeletonRoot{ nullptr };
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<glm::mat4> jointXForms;				//basic transforms
		std::vector<Node*> joints;
		std::vector<int32_t> jointHierarchy;
	};
	
	struct AnimationSampler {
		std::string interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec3> outputsVec4;
	};
	struct AnimationChannel {
		std::string path;
		Node* node;
		uint32_t samplerIndex;
	};
	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = 10000.f;
		float end = -10000.f;

	};
	std::vector<GLTFImage> images;
	std::vector<int32_t> textureIndices;
	std::vector<GLTFMaterial> materials;
	std::vector<Node*> nodes;
	std::vector<Skin> skins;
	std::vector<Animation> animations;
	std::vector<GLTFVertex>	vertices;
	std::vector<uint32_t> indices;
	std::vector<GLTFPrimitive> primitives;

	
	void loadImages(tinygltf::Model& model);
	void loadTextures(tinygltf::Model& model);
	void loadMaterials(tinygltf::Model& model);
	Node* findNode(Node* parent, uint32_t index);
	Node* nodeFromIndex(uint32_t index);
	void loadSkins(tinygltf::Model& model);
	void loadAnimations(tinygltf::Model& model);
	void loadNode(const tinygltf::Node& node, const tinygltf::Model& model, Node* parent, uint32_t nodeIndex);
public:
	GLTFLoader();
	~GLTFLoader();
	bool load(const char* filename, alertFunc alertF, void* ptr);
	const std::vector<GLTFVertex>& getVertices()const { return vertices; };
	const std::vector<uint32_t>& getIndices()const { return indices; }
	const std::vector<GLTFPrimitive>& getPrimitives()const { return primitives; }
	const std::vector<GLTFImage>& getImages()const { return images; }
	const std::vector<GLTFMaterial>& getMaterials()const { return materials; }
	//const std::vector<GLTFSkin>& getSkins()const { return skins; }
};
