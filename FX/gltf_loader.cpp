#define TINYGLTF_IMPLEMENTATION
//#include <tiny_gltf.h>
#include "gltf_loader.h"
#include <cassert>

GLTFLoader::GLTFLoader() {

}

GLTFLoader::~GLTFLoader() {

}

void GLTFLoader::loadImages(tinygltf::Model& model) {
	images.resize(model.images.size());
	for (size_t i = 0; i < model.images.size(); i++) {
		tinygltf::Image& image = model.images[i];
		uint32_t bufferSize = 0;
		GLTFImage img;
		img.width = image.width;
		img.height = image.height;
		img.pixels.resize(img.width * img.height * 4);
		if (image.component == 3) {
			//RGB, convert to RGBA
			uint8_t* rgba = img.pixels.data();
			uint8_t* rgb = &image.image[0];
			for (size_t i = 0; i < img.width * img.height; ++i) {
				memcpy(rgba, rgb, sizeof(uint8_t) * 3);
				rgba += 4;
				rgb += 3;
			}
		}
		else {
			memcpy(img.pixels.data(), &image.image[0], img.width * img.height * 4);
		}
		images.push_back(img);
	}
}

void GLTFLoader::loadTextures(tinygltf::Model& model) {
	textureIndices.resize(model.textures.size());
	for (size_t i = 0; i < model.textures.size(); ++i) {
		textureIndices[i] = model.textures[i].source;
	}
}

void GLTFLoader::loadMaterials(tinygltf::Model& model) {
	materials.resize(model.materials.size());
	for (size_t i = 0; i < model.materials.size(); ++i) {
		materials[i].metallic = 1.f;//blender doesn't seem to export 100% mettalic value, so set here, and if material has value it will override
		tinygltf::Material& mat = model.materials[i];
		for (auto& parm : mat.values) {
			if (parm.first == "baseColorFactor") {
				materials[i].baseColorFactor = glm::make_vec4(parm.second.ColorFactor().data());
			}
			else if (parm.first == "metallicFactor") {
				materials[i].metallic = parm.second.Factor();
			}
			else if (parm.first == "roughnessFactor") {
				materials[i].roughness = parm.second.Factor();
			}
			else if (parm.first == "baseColorTexture") {
				materials[i].baseColorTextureIndex = parm.second.TextureIndex();
			}

		}

	}
}

GLTFLoader::Node* GLTFLoader::findNode(Node* parent, uint32_t index) {
	GLTFLoader::Node* nodeFound{ nullptr };
	if (parent->index == index) {
		return parent;
	}
	for (auto& child : parent->children) {
		nodeFound = findNode(child, index);
		if (nodeFound)
			break;
	}
	return nodeFound;
}

GLTFLoader::Node* GLTFLoader::nodeFromIndex(uint32_t index) {
	Node* nodeFound = nullptr;
	for (auto& node : nodes) {
		nodeFound = findNode(node, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;
}

void GLTFLoader::loadSkins(tinygltf::Model& model) {
	skins.resize(model.skins.size());

	for (size_t i = 0; i < model.skins.size(); ++i) {
		tinygltf::Skin& skin = model.skins[i];
		//Find the root node of the skeleton
		skins[i].skeletonRoot = nodeFromIndex(skin.skeleton);
		skins[i].jointHierarchy.resize(skin.joints.size());
		memcpy(skins[i].jointHierarchy.data(), skin.joints.data(), sizeof(int32_t) * skin.joints.size());
		skins[i].jointXForms.resize(skin.joints.size());
		//Find joint nodes
		int32_t parentID = -1;
		int32_t idx = 0;
		glm::mat4 identity = glm::mat4(1.f);
		for (int jointIndex : skin.joints) {

			Node* node = nodeFromIndex(jointIndex);
			glm::mat4 xform = identity;

			if (node) {
				xform = glm::translate(identity, node->translation) * glm::mat4(node->rotation) * glm::scale(identity, node->scale) * node->matrix;
				skins[i].joints.push_back(node);
				if (node->parent && idx > 0) {
					parentID = (int32_t)std::distance(skin.joints.begin(), std::find(skin.joints.begin(), skin.joints.end(), node->parent->index));
					xform = skins[i].jointXForms[parentID] * xform;
				}


				skins[i].jointHierarchy[idx] = parentID;
			}

			skins[i].jointXForms[idx] = xform;
			idx++;
		}

		if (skin.inverseBindMatrices > -1) {
			const tinygltf::Accessor& accessor = model.accessors[skin.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			skins[i].inverseBindMatrices.resize(accessor.count);
			memcpy(skins[i].inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
		}
	}
}

void GLTFLoader::loadAnimations(tinygltf::Model& model) {
	animations.resize(model.animations.size());
	for (size_t i = 0; i < model.animations.size(); ++i) {
		tinygltf::Animation anim = model.animations[i];
		animations[i].name = anim.name;

		//Samplers
		animations[i].samplers.resize(anim.samplers.size());
		for (size_t j = 0; j < anim.samplers.size(); ++j) {
			tinygltf::AnimationSampler& sampler = anim.samplers[j];
			AnimationSampler& dst = animations[i].samplers[j];
			dst.interpolation = sampler.interpolation;
			//Read sampler keyframe input time values
			{
				const tinygltf::Accessor& accessor = model.accessors[sampler.input];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				const void* data = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				const float* buf = static_cast<const float*>(data);
				for (size_t index = 0; index < accessor.count; ++index) {
					dst.inputs.push_back(buf[index]);
				}
				//Adjust animation's start and end times
				for (auto input : animations[i].samplers[j].inputs) {
					if (input < animations[i].start) {
						animations[i].start = input;
					}
					if (input > animations[i].end) {
						animations[i].end = input;
					}
				}
			}
			//read sampler keyframe output translate/rotate/scale values
			{
				const tinygltf::Accessor& accessor = model.accessors[sampler.output];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				const void* data = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				switch (accessor.type) {
				case TINYGLTF_TYPE_VEC3: {
					const glm::vec3* buf = static_cast<const glm::vec3*>(data);
					for (size_t index = 0; i < accessor.count; index++) {
						dst.outputsVec4.push_back(glm::vec4(buf[index], 0.f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4: {
					const glm::vec4* buf = static_cast<const glm::vec4*>(data);
					for (size_t index = 0; i < accessor.count; index++) {
						dst.outputsVec4.push_back(buf[index]);
					}
					break;
				}
				}
			}
		}
		//Channels
		animations[i].channels.resize(anim.channels.size());
		for (size_t j = 0; j < anim.channels.size(); j++) {
			tinygltf::AnimationChannel& channel = anim.channels[j];
			AnimationChannel& dst = animations[i].channels[j];
			dst.path = channel.target_path;
			dst.samplerIndex = channel.sampler;
			dst.node = nodeFromIndex(channel.target_node);
		}
	}

}

void GLTFLoader::loadNode(const tinygltf::Node& modelNode, const tinygltf::Model& model, Node* parent, uint32_t nodeIndex) {
	Node* node = new Node{};
	node->parent = parent;
	node->index = nodeIndex;
	node->skin = modelNode.skin;
	node->name = modelNode.name;
	node->matrix = glm::mat4(1.f);

	if (modelNode.translation.size() == 3) {
		node->translation = glm::make_vec3(modelNode.translation.data());
	}
	if (modelNode.rotation.size() == 4) {
		glm::quat q = glm::make_quat(modelNode.rotation.data());
		node->rotation = glm::mat4(q);
	}
	if (modelNode.scale.size() == 3) {
		node->scale = glm::make_vec3(modelNode.scale.data());
	}
	if (modelNode.matrix.size() == 16) {
		node->matrix = glm::make_mat4x4(modelNode.matrix.data());
	}

	//Load node's children
	if (modelNode.children.size() > 0) {
		for (size_t i = 0; i < modelNode.children.size(); i++) {
			loadNode(model.nodes[modelNode.children[i]], model, node, modelNode.children[i]);
		}
	}

	//If the node contains mesh data, we load vertices and indices from the buffers
	//In glTF this is done via accessors and buffer views
	if (modelNode.mesh > -1) {
		const tinygltf::Mesh mesh = model.meshes[modelNode.mesh];
		//Iterate through all primitives of this node's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			const tinygltf::Primitive& primitive = mesh.primitives[i];
			uint32_t	startIndex = static_cast<uint32_t>(indices.size());
			uint32_t	vertexStart = static_cast<uint32_t>(vertices.size());
			uint32_t	indexCount = 0;
			bool		hasSkin = 0;
			//Vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const uint16_t* jointIndicesBuffer = nullptr;
				const float* jointWeightsBuffer = nullptr;
				size_t vertexCount = 0;

				//Get buffer data for vertex normals
				if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
					vertexCount = accessor.count;
				}
				//Get buffer data for vertex normals
				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					normalBuffer = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
				}
				//get buffer data for texture coords
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
				}
				//get vertex joint indices
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					jointIndicesBuffer = reinterpret_cast<const uint16_t*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
				}
				//get vertex joint weights
				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					jointWeightsBuffer = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
				}

				hasSkin = (jointIndicesBuffer && jointWeightsBuffer);

				//Append data to model's vertex buffer
				for (size_t v = 0; v < vertexCount; v++) {
					GLTFVertex vert{};
					vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
					vert.norm = glm::normalize(glm::vec3(normalBuffer ? glm::make_vec3(&normalBuffer[v * 3]) : glm::vec3(0.f)));
					vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.f);
					vert.color = glm::vec3(1.f);
					vert.jointIndices = hasSkin ? glm::vec4(glm::make_vec4(&jointIndicesBuffer[v * 4])) : glm::vec4(0.f);
					vert.jointWeights = hasSkin ? glm::make_vec4(&jointWeightsBuffer[v * 4]) : glm::vec4(0.f);
					vertices.push_back(vert);
				}
			}
			//indices
			{
				const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				//glTF supports different component types of indices
				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					uint32_t* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + vertexStart);
					}
					delete[]buf;
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					uint16_t* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + vertexStart);
					}
					delete[]buf;
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					uint8_t* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indices.push_back(buf[index] + vertexStart);
					}
					delete[]buf;
					break;
				}
				default:
					return;
				}
			}
			GLTFPrimitive prim{};
			prim.indexStart = startIndex;
			prim.indexCount = indexCount;
			prim.material = primitive.material;
			primitives.push_back(prim);
		}
	}

	if (parent) {
		parent->children.push_back(node);
	}
	else {
		nodes.push_back(node);
	}
}



bool GLTFLoader::load(const char* filename, alertFunc alert, void* ptr) {
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

	if (!warn.empty())
		alert(asWARNING, warn.c_str(), ptr);
	if (!err.empty())
		alert(asERROR, err.c_str(), ptr);
	assert(res);
	loadImages(model);
	loadMaterials(model);
	loadTextures(model);
	const tinygltf::Scene& scene = model.scenes[model.defaultScene];

	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
		const tinygltf::Node& node = model.nodes[scene.nodes[i]];
		loadNode(node, model, nullptr, scene.nodes[i]);

	}
	loadSkins(model);
	loadAnimations(model);

	return res;

}