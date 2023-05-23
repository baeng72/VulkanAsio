#include <windows.h>
#include  <VulkanLib.h>
#include <uilib.h>
#include "AsioMgr.h"
#include <string>
#include <queue>
#include "fxobjects.h"
#include "valves.h"
#include "camera.h"
#include "gltf_loader.h"
#include <chrono>


//struct HandlerBase {
//	virtual void handle(std::vector<float> elements) = 0;
//};
//
//template <class Derived>
//struct Handler : HandlerBase {
//	Derived d;
//	void handle(std::vector<float> elements)override {
//		for (float f : elements) {
//			d.handle(f);
//		}
//	}
//};
//
//struct HandlerComp {
//	void handle(float f) {
//	}
//};
//
//struct HandlerDist {
//	void handle(float f) {
//
//	}
//};

//settings
constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.f, 10.f, 3.f),glm::vec3(0.f,1.f,0.f),YAW,-75.f);//check y
float lastX = SCR_WIDTH / 2.f;
float lastY = SCR_HEIGHT / 2.f;
bool firstMouse = true;

//timing
float deltaTime = 0.f;
float lastFrame = 0.f;

struct Primitive {
	uint32_t vertexStart;
	uint32_t indexStart;
	uint32_t indexCount;
	uint32_t materialIndex;
};
struct UBO {
	glm::mat4 projection;
	glm::mat4 view;	
	glm::vec3 camPos;
	float ao;
	//glm::vec3 albedo;
	//float roughness;
	
};

struct Light {
	glm::vec3 lightPosition;
	float padding;
	glm::vec3 lightColor;
	float padding2;
};

struct Lights {
	Light lights[4];
};

struct PushConst {
	glm::mat4 model;
	glm::vec3 albedo;
	float metallic;
	float roughness;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct Material {
	glm::vec3 albedo;
	float metallic;
	float roughness;

};

struct Mesh {
	int primitiveStart;
	int materialStart;
	int count;	
};

struct ReverbParams {

	//audio data
	enum PresetType{Factory,WarmRoom,LargeHall,BrightClub,SmallClub,TinyRoom};
	float reverbTime;
	float damping;
	float lowShelfFc;
	float lowShelfGain;
	float hiShelfFc;
	float hiShelfGain;
	float wetLevelDb;
	float dryLevelDb;
	float apfDelayMax;
	float apfDelayWeight;	
	float fixedDelayMax;
	float fixedDelayWeight;
	float preDelayTime;
	int		density;
	PresetType type;
	ReverbParams() {
		Preset(Factory);
	}
	void Preset(PresetType preset) {
		switch (preset) {
		case Factory:
			reverbTime = 0.5f;
			damping = 0.5f;
			lowShelfFc = 500.f;
			lowShelfGain = 0.f;
			hiShelfFc = 2000.f;
			hiShelfGain = 0.f;
			wetLevelDb = -3.f;
			dryLevelDb = -3.f;
			apfDelayMax = 5.f;
			apfDelayWeight = 50.f;
			fixedDelayMax = 5.f;
			fixedDelayWeight = 50.f;
			preDelayTime = 25.f;
			density = 0;
			break;

		case WarmRoom:
			reverbTime = 0.856598f;
			damping = 0.4f;
			lowShelfFc = 144.f;
			lowShelfGain = -20.f;
			hiShelfFc = 2000.f;
			hiShelfGain = -8.f;
			wetLevelDb = -10.f;
			dryLevelDb = 0.f;
			apfDelayMax = 33.f;
			apfDelayWeight = 84.f;
			fixedDelayMax = 82.f;
			fixedDelayWeight = 84.f;
			preDelayTime = 25.f;
			density = 0;
			break;
		case LargeHall:
			reverbTime = 0.901248f;
			damping = 0.3f;
			lowShelfFc = 150.f;
			lowShelfGain = -20.f;
			hiShelfFc = 4000.f;
			hiShelfGain = -6.f;
			wetLevelDb = -12.f;
			dryLevelDb = 0.f;
			apfDelayMax = 33.f;
			apfDelayWeight = 85.f;
			fixedDelayMax = 81.f;
			fixedDelayWeight = 100.f;
			preDelayTime = 25.f;
			density = 0;
			break;
		case BrightClub:
			reverbTime = 0.849129f;
			damping = 0.53f;
			lowShelfFc = 500.f;
			lowShelfGain = -3.f;
			hiShelfFc = 4000.f;
			hiShelfGain = 6.f;
			wetLevelDb = -15.f;
			dryLevelDb = 0.f;
			apfDelayMax = 33.f;
			apfDelayWeight = 85.f;
			fixedDelayMax = 81.f;
			fixedDelayWeight = 80.f;
			preDelayTime = 10.f;
			density = 1;
			break;
		case SmallClub:
			reverbTime = 0.849129f;
			damping = 0.535f;
			lowShelfFc = 500.f;
			lowShelfGain = -3.f;
			hiShelfFc = 4000.f;
			hiShelfGain = -6.f;
			wetLevelDb = -15.f;
			dryLevelDb = 0.f;
			apfDelayMax = 33.f;
			apfDelayWeight = 85.f;
			fixedDelayMax = 81.f;
			fixedDelayWeight = 80.f;
			preDelayTime = 10.f;
			density = 1;
			break;
		case TinyRoom:
			reverbTime = 0.614246f;
			damping = 0.27f;
			lowShelfFc = 150.f;
			lowShelfGain = -20.f;
			hiShelfFc = 4000.f;
			hiShelfGain = 0.f;
			wetLevelDb = -12.f;
			dryLevelDb = 0.f;
			apfDelayMax = 37.f;
			apfDelayWeight = 23.27f;
			fixedDelayMax = 87.5f;
			fixedDelayWeight = 100.f;
			preDelayTime = 30.5f;
			density = 0;
			break;
		}
		type = preset;
	}
	static std::vector<const char*>getPresets() {
		return std::vector<const char*> {
			"Factory",
				"Warm Room",
				"Large Hall",
				"Bright Club",
				"Small Club",
				"Tiny Room",

		};
	}
	static std::vector<const char*>getDensities() {
		return std::vector<const char*>{
			"thick",
			"thin"
		};
	}
};

struct PreampParams {
	float inputLevelDb;
	float saturation;
	float lowShelfFc;
	float lowShelfGain;
	float hiShelfFc;
	float hiShelfGain;
	float outputLevelDb;
	PreampParams() {
		reset();
	}
	void reset() {
		inputLevelDb = -6.f;
		saturation = 2.f;
		lowShelfFc = 500.f;
		lowShelfGain = 0.f;
		hiShelfFc = 2000.f;
		hiShelfGain = 0.f;
		outputLevelDb = 0.f;
	}
};

struct PhaserParams {
	float lfoRate;
	float lfoDepth;
	float intensity;
	PhaserParams() {
		reset();
	}
	void reset() {
		lfoRate = 0.2f;
		lfoDepth = 50.f;
		intensity = 50.f;
	}
};

enum SoundTypes{snare,cowbell,bassdrum ,maxsounds};

class FXApp : public SDLApp {
	//vulkan stuff
	std::unique_ptr<VulkSwapchain> swapchain;
	u32 currFrame;
	std::unique_ptr<VulkanVIBuffer> indexBufferPtr;
	std::unique_ptr<VulkanVIBuffer> vertexBufferPtr;
	Vulkan::Buffer vertexBuffer;
	Vulkan::Buffer indexBuffer;
	u32 indexCount;
	std::vector<Primitive> primitives;
	std::unique_ptr<VulkanUniformBuffer> uniformBufferPtr;
	Vulkan::Buffer uniformBuffer;
	UBO* pUBO{ nullptr };
	std::unique_ptr<VulkanUniformBuffer> storageBufferPtr;
	Vulkan::Buffer storageBuffer;
	Lights* pLights{ nullptr };
	std::vector<Material> materials;
	std::unique_ptr<VulkanDescriptor> descriptor;
	VkDescriptorSetLayout descriptorLayout;
	VkDescriptorSet descriptorSet;
	std::unique_ptr<VulkanPipelineLayout> pipelineLayoutPtr;
	VkPipelineLayout pipelineLayout;
	std::unique_ptr<VulkanPipeline> pipelinePtr;
	VkPipeline pipeline;
	std::unique_ptr<UI>	ui;
	std::vector<Mesh> meshes;
	static void loadErr(AlertSeverity as, const char* msg, void* ptr);
	void BuildUniformBuffers(VulkContext& context);
	void BuildDescriptors(VulkContext& context);
	void BuildPipeline(VulkContext& context);
	//audio stuff
	enum EffectType{etNone,etDynamics,etSuperSaturation,etModulatedDelay,etStereoDelay,etReverb,etPreamp,etPhaser};	
	static void audioCallback(float** inputBuffers, int inputCount, float** outputBuffers, int outputCount, int samples, void* userPtr);
	void asioProcess(float** inputBuffers, int inputCount, float** outputBuffers, int outputCount, int samples);	
	std::unique_ptr<AsioDriverMgr> asioDrvrMgr;
	std::unique_ptr<AsioMgr> asioMgr;
	std::vector<const char*>asioDeviceList;
	std::vector<const char*>inputChannelNames;
	std::vector<const char*> outputChannelNames;
	int selectedDevice{ -1 };
	int inputChannel{ -1 };
	std::vector<int> outputChannels;
	ASIOSampleRate sampleRate;
	bool showASIOPopup{ false };
	bool isAsioReady{ false };
	bool isAsioStarted{ false };
	int sampleSize;
	int asioMemSize;
	int asioMemBytes;
	void* inputBuffers[2];
	void* outputBuffers[2];
	void* outputBuffers2[2];
	std::vector<float> nextOutput;
	std::vector<float> convertBuffer;
	long inputLatency;
	long outputLatency;
	float fGain{ 0.f };
	std::vector<EffectType> effectsList;
	std::vector<EffectType> threadEffects;
	std::atomic<bool> applyCompression{ false };
	float fCompThreshold{ -40.f };
	float fCompRatio{ 15.f };
	//compressor setup
	float compInputGainDB;
	float compThresholdDB;
	float compAttack;
	float compRelease;
	float compRatio;
	float compOutputGainDB;
	float compKneeWidthDB;
	bool compLimitGate;
	bool compSoftKnee;
	bool compStereoLink;
	int compMode;
	DynamicsProcessor dynamicsProcessor;
	std::atomic<bool> applySuperSat{ false };
	
	
	float ssatInputTrim;
	float ssatDistortionControl;
	float ssatToneControl;
	float ssatVolumeControl;
	bool ssatTurbo;
	bool ssatBypass;
	TurboDistorto distortion;
	std::atomic<bool> applyModDelay{ false };
	float modDelayRate{ 0.f };
	float modDelayDepth{ 0.f };
	float modDelayFeedback{ 0.f };
	int modDelayEffect{ 0 };
	ModulatedDelay modulatedDelay;
	std::atomic<bool> applyStereoDelay{ false };

	float sdelDelayTime{ 0.f };
	float sdelFeedback{ 0.f };
	float sdelRatio{ 0.f };
	int sdelAlgorithm{ 0 };
	float sdelWetLevel{ 0.f };
	float sdelDryLevel{ 0.f };
	AudioDelay audioDelay;
	std::atomic<bool> applyReverb{ false };
	ReverbParams reverbParams;
	ReverbTank reverbTank;
	std::atomic<bool> applyPreamp{ false };
	PreampParams preampParams;
	ClassATubePre preamp;
	std::atomic<bool> applyPhaser{ false };
	PhaserParams phaserParams;
	PhaseShifter phaseShifter;
	bool showUI{ true };
	float windowYPos{ 0.f };	
	std::vector<float> waveFileData;
	bool recording{ false };
	std::vector<std::vector<float>> sounds;
	std::vector<bool> soundPlaying;
	std::vector<int> soundPos;
	std::atomic<bool> applyMetronome{ false };
	int metroBPM{ 60 };
	std::chrono::high_resolution_clock::time_point lastTime;
	virtual void Update(float)override;
	virtual void Draw(float)override;
	virtual void Resize()override;
	virtual void SDLEvent(const SDL_Event* event)override;
	virtual void OnExit()override;
	void CreateSwapchain();
	void loadConfig(const char* cfgFile);
	void saveConfig(const char* cfgFile);
	void setAsioDriver(int drvNum);
	void setAsioDeviceSettings(int inputC, std::vector<int>&outputChs);
	void  ToFloatInt32(int32_t*inputBuffer);//copy from output buffer to convert
	void FromFloatInt32(int32_t*outputBuffer);//copy from convert buffer to output buffer;
	float db2lin(float db) { return powf(10.f, 0.05f * db); }
	
	float lin2db(float lin) { return 20.f * log10f(lin); }
	void processCompression(float* inputBuffer, float* outputBuffer, DynamicsProcessorParameters& params);
	float drawCompressor(float);
	float drawSuperSaturation(float);
	float drawModDelay(float);
	float drawStereoDelay(float);
	float drawReverb(float);
	float drawPreamp(float);
	float drawPhaser(float);
	float drawMetronome(float);
	void queueEffect(EffectType type, bool add);
	void saveWAVFile();
	void loadRawFile(const char* fileName, std::vector<float>& data);
public:
	FXApp();
	~FXApp();
	virtual bool Initialize(const char* title, i32 width, i32 height, bool resizeable = true)override;
};

FXApp::FXApp() {

}

FXApp::~FXApp() {
	saveConfig("config.txt");
	for (auto dev : asioDeviceList) {
		delete[] dev;
	}
	ui.reset();
	pipelinePtr.reset();
	pipelineLayoutPtr.reset();
	storageBufferPtr.reset();
	uniformBufferPtr.reset();
	indexBufferPtr.reset();
	vertexBufferPtr.reset();
	swapchain.reset();
	vulkState.Cleanup();
}

//load raw wave data and convert to float [-1,1]
void FXApp::loadRawFile(const char* pRaw,std::vector<float>&data) {
	//load 16-bit raw data
	FILE* fp = fopen(pRaw, "rb");
	fseek(fp, 0, SEEK_END);
	int size = (int)ftell(fp);
	int words = size / 2;
	int16_t* pd = new int16_t[words];//2 bytes per 16 bit 
	fseek(fp, 0, SEEK_SET);
	fread(pd, 2, words, fp);
	fclose(fp);
	
	//convert to floats
	float invdiv = 1 / (float)INT16_MAX;
	float hz = 22050;
	float dsthz = (float)sampleRate;
	float fac = dsthz / hz;
	int intfac = (int)fac;
	if (intfac < 1)
		intfac = 1;
	int newwords = words * intfac;
	data.resize(newwords);
	int i2 = 0;
	for (int i = 0; i < words; i++) {
		int16_t isample = pd[i];
		int16_t isamplele = isample >> 8 &0xFF | isample << 8;
		float fsample = isamplele * invdiv;
		for(int j=i2;j<i2+intfac;j++)
			data[j] = fsample;
		i2 += intfac;

	}
	delete[]pd;//could just use vector?


}
//save buffered data to a wav file. 
void FXApp::saveWAVFile() {
	FILE* fp = fopen("save.wav", "wb");
	if (fp) {
		char buffer[256];
		strcpy_s(buffer, "RIFF----WAVE");//riff chunk, fill length later
		fwrite(buffer, sizeof(char), strlen(buffer), fp);
		fwrite("fmt ", 1, 4, fp);//fmt chunk
		int32_t i = 16;
		fwrite(&i, sizeof(int32_t), 1, fp);//normal format size (16 bytes);
		PCMWAVEFORMAT fmt{};
		fmt.wf.wFormatTag = WAVE_FORMAT_PCM;
		int numchannels = 1;
		fmt.wf.nChannels = numchannels;//mono at moment
		fmt.wf.nSamplesPerSec = (DWORD)sampleRate;
		fmt.wf.nAvgBytesPerSec = ((DWORD)sampleRate * 16 * numchannels) / 8;
		fmt.wf.nBlockAlign = 2;
		fmt.wBitsPerSample = 16;
		fwrite(&fmt, sizeof(fmt), 1, fp);
		size_t dataPos = ftell(fp)+4;
		strcpy_s(buffer, "data----");
		fwrite(buffer, sizeof(char), strlen(buffer), fp);
		float mul = (float)INT16_MAX;//convert to 16-bit int from float [-1,1].
		for (auto sample : waveFileData) {
			int16_t conv = (int16_t)( sample *mul);
			fwrite(&conv, sizeof(int16_t), 1, fp);
		}
		size_t len = ftell(fp);
		fseek(fp, (long)dataPos,SEEK_SET);
		i =(int32_t)( len - dataPos);
		fwrite(&i, sizeof(int32_t), 1, fp);
		fseek(fp, 4, SEEK_SET);
		i = (int32_t)len;
		fwrite(&i, sizeof(int32_t), 1, fp);
		fclose(fp);
	}
}

bool FXApp::Initialize(const char* title, i32 width, i32 height, bool resizeable) {
	if (!SDLApp::Initialize(title, width, height, resizeable))
		return false;
	{
		VulkStateInitFlags flags{};
		//flags.enableValidation = false;
		//flags.enableWireframe = true;
		//flags.enableLineWidth = true;
		flags.enableGeometryShader = true;
		bool res = vulkState.Init(flags, mainWindow);
		assert(res);
	}
	{
		swapchain = std::make_unique<VulkSwapchain>();
		CreateSwapchain();
	}
	{
		VulkContext context = vulkState.getContext();
		ui = std::make_unique<UI>();
		ui->Init(context, swapchain->getRenderPass(), "../Shaders");
		ui->Resize(clientWidth, clientHeight);
	}
	VulkContext context = vulkState.getContext();
	{
		
			//loader = std::make_unique<GLTFLoader>();
			std::vector<Vertex> verts;
			std::vector<u32> inds;
			{
				std::unique_ptr<GLTFLoader> loader = std::make_unique<GLTFLoader>();
				loader->load("../Resources/Meshes/Phaser.gltf", loadErr, this);
				auto& vertices = loader->getVertices();
				auto& indices = loader->getIndices();
				auto& inprimitives = loader->getPrimitives();
				auto& images = loader->getImages();
				auto& inmaterials = loader->getMaterials();

				size_t matSize = materials.size();
				size_t primSize = primitives.size();
				for (auto& mat : inmaterials) {
					Material m;
					m.albedo = glm::vec3(mat.baseColorFactor);
					m.metallic = mat.metallic;
					m.roughness = mat.roughness;
					materials.push_back(m);
				}
				
				for (auto& v : vertices) {
					Vertex vert;
					vert.pos = v.pos;
					vert.normal = v.norm;
					vert.uv = v.uv;
					verts.push_back(vert);
				}
				inds.insert(inds.end(), indices.begin(), indices.end());
				for (auto& prim : inprimitives) {
					this->primitives.push_back({ 0,prim.indexStart,prim.indexCount,prim.material + (u32)matSize });
				}
				Mesh mesh{ (int)primSize,(int)matSize,(int)inprimitives.size() };
				meshes.push_back(mesh);
			}
			{
				std::unique_ptr<GLTFLoader> loader = std::make_unique<GLTFLoader>();
				loader->load("../Resources/Meshes/PedalBoard.gltf", loadErr, this);
				auto& vertices = loader->getVertices();
				auto& indices = loader->getIndices();
				auto& inprimitives = loader->getPrimitives();
				auto& images = loader->getImages();
				auto& inmaterials = loader->getMaterials();

				size_t matSize = materials.size();
				size_t primSize = primitives.size();
				size_t indStart = inds.size();
				size_t vertStart = verts.size();
				for (auto& mat : inmaterials) {
					Material m;
					m.albedo = glm::vec3(mat.baseColorFactor);
					m.metallic = mat.metallic;
					m.roughness = mat.roughness;
					materials.push_back(m);
				}

				for (auto& v : vertices) {
					Vertex vert;
					vert.pos = v.pos;
					vert.normal = v.norm;
					vert.uv = v.uv;
					verts.push_back(vert);
				}
				inds.insert(inds.end(), indices.begin(), indices.end());
				for (auto& prim : inprimitives) {
					this->primitives.push_back({ (u32)vertStart,prim.indexStart+(u32)indStart,prim.indexCount,prim.material + (u32)matSize });
				}
				Mesh mesh{ (int)primSize,(int)matSize,(int)inprimitives.size() };
				meshes.push_back(mesh);

			}
			VkDeviceSize vertSize = sizeof(Vertex) * verts.size();
			VkDeviceSize indSize = sizeof(uint32_t) * inds.size();
			std::vector<uint32_t> vertexLocations;
			VertexBufferBuilder::begin(context.device, context.queue, context.commandBuffer, context.memoryProperties)
				.AddVertices(vertSize, (float*)verts.data())
				.build(vertexBuffer, vertexLocations);
			vertexBufferPtr = std::make_unique<VulkanVIBuffer>(context.device, vertexBuffer, vertexLocations);

			std::vector<uint32_t> indexLocations;
			IndexBufferBuilder::begin(context.device, context.queue, context.commandBuffer, context.memoryProperties)
				.AddIndices(indSize, (uint32_t*)inds.data())
				.build(indexBuffer, indexLocations);
			indexBufferPtr = std::make_unique < VulkanVIBuffer>(context.device, indexBuffer, indexLocations);
			
	}
	{
		BuildUniformBuffers(context);
		//setup ubo
		
		pUBO->ao = 1.f;
		pUBO->projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.f);
		pUBO->projection[1][1] *= -1;
		//setup lights
		pLights->lights[0].lightPosition = glm::vec3(0.f, 10.f, 0.f);
		pLights->lights[1].lightPosition = glm::vec3(10.f, 10.f, 10.f);
		pLights->lights[2].lightPosition = glm::vec3(-10.f, -10.f, 10.f);
		pLights->lights[3].lightPosition = glm::vec3(10.f, -10.f, 10.f);

		pLights->lights[0].lightColor = glm::vec3(50.f);
		pLights->lights[1].lightColor = glm::vec3(300.f);
		pLights->lights[2].lightColor = glm::vec3(300.f);
		pLights->lights[3].lightColor = glm::vec3(300.f);
	}
	{
		BuildDescriptors(context);
	}
	{
		BuildPipeline(context);
	}
	{
		asioDrvrMgr = std::make_unique<AsioDriverMgr>();
		int numDev = asioDrvrMgr->asioGetNumDev();
		for (auto dev : asioDeviceList) {
			delete[] dev;
		}
		asioDeviceList.clear();
		for (int i = 0; i < numDev; ++i) {
			char name[64];
			asioDrvrMgr->asioGetDriverName(i, name, sizeof(name));
			auto len = strlen(name);
			char* p = new char[len + 1];
			strcpy_s(p, len + 1, name);
			asioDeviceList.push_back(p);

		}
	}
	{
		{
			//compression defaults
			compThresholdDB = -10.f;
			compKneeWidthDB = 10.f;
			compRatio = 1.f;
			compAttack = 5.f;
			compRelease = 500.f;
			//compInputGainDB = 0.f;// compThresholdDB = 0.f;



			compOutputGainDB = 0.f;// compKneeWidthDB = 0.f;
			compLimitGate = compSoftKnee = compStereoLink = false;
			compMode = 0;
			//dynamicsProcessor.reset(sampleRate);
		}
		{
			ssatInputTrim = 5.f;
			ssatVolumeControl = 5.f;
			ssatDistortionControl = 5.f;
			ssatTurbo = false;
			ssatBypass = false;
			//distortion.reset(sampleRate);
		}
		{
			modDelayRate = 0.2f;
			modDelayDepth = 50.f;
			modDelayFeedback = 50.f;
			modDelayEffect = 0;
		}
		{
			sdelDelayTime = 250.f;
			sdelFeedback = 50.f;
			sdelRatio = 50.f;
			sdelWetLevel = -3.f;
			sdelDryLevel = -3.f;
		}
		{
			
		}
		effectsList.reserve(10);
		threadEffects.resize(20);//enough space reserved?
		loadConfig("config.txt");
	}
	{
		sounds.resize(maxsounds);
		soundPlaying.resize(maxsounds, false);
		soundPos.resize(maxsounds, 0);
		loadRawFile("../raw/snardrum.raw", sounds[snare]);
		loadRawFile("../raw/cowbell1.raw", sounds[cowbell]);
		loadRawFile("../raw/bassdrum.raw", sounds[bassdrum]);
		
	}
	return true;
}

void FXApp::loadErr(AlertSeverity as, const char* msg, void* ptr) {
	FXApp* pThis = (FXApp*)ptr;

}

void FXApp::loadConfig(const char* cfgFile) {
	std::vector<int> outputChs;
	std::ifstream file(cfgFile);
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {
			//line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
			if (line[0] == '#' || line.empty())
				continue;
			auto delimeterPos = line.find("=");
			auto name = line.substr(0, delimeterPos);
			auto value = line.substr(delimeterPos + 1);
			//setup values
			if (name == "AsioDevice") {
				for (int i = 0; i < (int)asioDeviceList.size(); i++) {
					if (value == asioDeviceList[i]) {
						setAsioDriver(i);
						
						break;
					}
				}
			}
			else if (name == "InputChannel") {
				inputChannel = std::stoi(value);
			}
			else if (name == "OutputChannel") {
				outputChs.push_back(std::stoi(value));
			}
			else if (name == "OutputChannel1") {
			//	outputChs.push_back(std::stoi(value));
			}
			
		}
	}
	setAsioDeviceSettings(inputChannel, outputChs);
}

void FXApp::saveConfig(const char* cfgFile) {
	std::ofstream file(cfgFile);
	if (selectedDevice != -1) {
		file << "#Device Settings" << std::endl;
		file << "AsioDevice=" << asioDeviceList[selectedDevice] << std::endl;
		if (inputChannel != -1) {
			file << "InputChannel=" << inputChannel << std::endl;
		}
		if (outputChannels.size() > 0) {			
				file << "OutputChannel=" << outputChannels[0] << std::endl;
				if (outputChannels.size() > 1) {
					file << "OutputChannel1=" << outputChannels[1] << std::endl;
			}
		}
	}
}

void FXApp::setAsioDriver(int drvNum) {
	selectedDevice = drvNum;
	asioDrvrMgr->loadDriver((char*)asioDeviceList[selectedDevice]);
	asioMgr = std::make_unique<AsioMgr>(nullptr);
	int inputChannels = asioMgr->getNumInputChannels();
	inputChannelNames.resize(inputChannels);
	inputChannel = -1;
	for (int i = 0; i < inputChannels; i++) {
		inputChannelNames[i] = asioMgr->getInputChannelName(i);
	}
	int outputChannelCount = asioMgr->getNumOutputChannels();
	outputChannelNames.resize(outputChannelCount);
	outputChannels.clear();
	for (int i = 0; i < outputChannelCount; i++) {
		outputChannelNames[i] = asioMgr->getOutputChannelName(i);
	}
	sampleRate = asioMgr->getSampleRate();
	preamp.reset(sampleRate);
	distortion.reset(sampleRate);
	dynamicsProcessor.reset(sampleRate);
	modulatedDelay.reset(sampleRate);
	audioDelay.reset(sampleRate);
	audioDelay.createDelayBuffers(sampleRate, 2000.0);

	reverbTank.reset(sampleRate);
	phaseShifter.reset(sampleRate);
	asioMemSize = asioMgr->GetMemSize();

	ASIOError err = ASIOFuture(kAsioCanTimeInfo, nullptr);
	if(ASE_SUCCESS==err){
		err = ASIOFuture(kAsioEnableTimeCodeRead, nullptr);
	}	
}
void FXApp::setAsioDeviceSettings(int inputC,std::vector<int>&outputChs){
	inputChannel = inputC;
	outputChannels = outputChs;
	isAsioReady = selectedDevice != -1 && inputChannel != -1 && outputChannels.size()>0&&outputChannels[0]!=-1;
	if (isAsioReady) {
		std::vector<long> iChannels{ inputChannel };
		std::vector<long> oChannels;
		oChannels.insert(oChannels.begin(), outputChannels.begin(), outputChannels.end());
		asioMgr->InitBuffers(iChannels, oChannels);
		asioMemBytes = asioMgr->GetMemBytes();		
		convertBuffer.resize(asioMemSize,0.f);
		asioMgr->getLatencies(&inputLatency, &outputLatency);
		std::cout << "Main Thread id: " << GetCurrentThreadId() << std::endl;		
	}
}
//const double fScaler32 = (double)0x7fffffffL;

//convert from 32-bit integer to floating point [-1,1]
void FXApp::ToFloatInt32(int32_t*inputBuffer) {
	/*float rmax = (float)INT32_MAX;
	float rmin = (float)INT32_MIN;
	float tmax = 1.f;
	float tmin = -1.f;c
	float div = 1.f / (rmax - rmin);*/
	float uintmaxinv = 1.f/(float)INT32_MAX;
	//double sc = fScaler32 + 0.49999;
	//double scinv = 1 / sc;
	//const double g = 1.0 / 0x7fffffff;
	//const char* src = (const char*)inputBuffer;
	for (int i = 0; i < asioMemSize; i++) {
		float sample = (float)inputBuffer[i];		
		//float conv= (sample - rmin) * div * (tmax - tmin) + tmin;
		float conv2 = sample * uintmaxinv;
		//float conv4 = (float)(g * sample);
		//assert(conv2 == conv4);
		//float conv3 = sample * scinv;
		convertBuffer[i] = conv2;
	}

	
}
//convert from floating point [-1,1] to 32-bit integer
void FXApp::FromFloatInt32(int32_t* outputBuffer) {
	//float rmax = 1.f;
	//float rmin = -1.f;
	//float tmax = (float)INT32_MAX;
	//float tmin = (float)INT32_MIN;
	//float div = 1.f / (rmax - rmin);
	float uintmax = (float)INT32_MAX;
	
	//double sc = fScaler32 + 0.49999;
	for (int i = 0; i < asioMemSize; i++) {
		float sample = convertBuffer[i];// convertBuffer[i];
	
		//assert(!isnan(sample));
		//int conv = (int32_t)((sample - rmin) * div * (tmax - tmin) + tmin);
		int32_t conv2 =(int32_t)( sample * uintmax);
	
		//float conv3 = (float)((double)sample * sc);
		outputBuffer[i] = conv2;
	}
	
}


void FXApp::audioCallback(float**inputBuffers,int inputCount,float ** outputBuffers,int outputCount,int samples,void*userPtr){
	FXApp* pThis = (FXApp*)userPtr;
	return pThis->asioProcess(inputBuffers, inputCount, outputBuffers, outputCount, samples);

}
//#define __DIRECT__PASSTHROUGH__
//#define TEST_INPUT_NOISE

void FXApp::asioProcess(float**inputBuffers,int inputCount,float**outputBuffers,int outputCount,int samples){
	size_t fxcnt = 0;
	
	for (fxcnt; fxcnt < effectsList.size(); fxcnt++) {
		threadEffects[fxcnt] = effectsList[fxcnt];
	}
#ifdef __DIRECT__PASSTHROUGH__
	int32_t* pinput = (int32_t*)inputBuffer;
	int32_t* poutput = (int32_t*)outputBuffer;
	for (int i = 0; i < asioMemSize; i++) {
		poutput[i] = pinput[i];
		

	}
	
	//memcpy(outputBuffer, inputBuffer, asioMemBytes);
#else

	
#ifdef TEST_OUTPUT_NOISE

	double theta = 1000 * 2 * kPi / (double)sampleRate;
	int vol = INT32_MAX / 512;
	double ampl = vol >> 2;
	uint32_t* pout = (uint32_t*)outputBuffer;
	for (int i = 0; i < asioMemSize; i++) {
		uint32_t sample = (uint32_t)(ampl * std::sin(theta * (double)i));
		pout[i] = sample;
	}



#endif
	
#ifdef TEST_INPUT_NOISE
	
		
	double theta = 523.25 * 2 * kPi / (double)sampleRate;
	//double theta2 = 659.25 * 2 * kPi / (double)sampleRate;
	int vol = INT32_MAX / 512;
	double ampl = vol >> 2;
	uint32_t* pout = (uint32_t*)inputBuffer;
	for (int i = 0; i < asioMemSize; i++) {
		uint32_t sample = (uint32_t)(ampl * std::sin(theta * (float)i));
		//uint32_t sample2 = (uint32_t)(ampl * std::sin(theta2 * (double)i));
		pout[i] = sample;// (sample + sample2) / 2.f;
	}

	
#endif
	//ToFloatInt32((int32_t*)inputBuffer);
	float* inputBuffer = inputBuffers[0];
	
	memcpy(&convertBuffer[0], inputBuffer,samples*sizeof(float));
	
	float* outputBuffer = outputBuffers[0];
	
	//std::unique_ptr<HandlerBase> handler;
	for (auto fx : threadEffects) {
		switch (fx) {
		case etDynamics: {
			//handler = std::make_unique<Handler<HandlerComp>>();
			//handler->handle(convertBuffer);
			DynamicsProcessorParameters params{};
			params.ratio = compRatio;
			params.attackTime_mSec = compAttack;
			params.enableSidechain = false;
			params.threshold_dB = compThresholdDB;
			params.kneeWidth_dB = compKneeWidthDB;
			params.releaseTime_mSec = compRelease;
			params.outputGain_dB = compOutputGainDB;
			params.calculation = (dynamicsProcessorType)compMode;
			params.hardLimitGate = compLimitGate;
			params.softKnee = compSoftKnee;
			dynamicsProcessor.setParameters(params);
			//dynamicsProcessor.setSampleRate(sampleRate);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];

				float outsample = (float)dynamicsProcessor.processAudioSample(sample);
				//dynamicsProcessor.processAudioFrame(&sample, &outsample, 1, 1);
				convertBuffer[i] = outsample;
			}
		}

					   break;

		case etSuperSaturation: {
			//super saturation
			TurboDistortoParameters params{};
			params.bypassFilter = ssatBypass;
			params.distortionControl_010 = ssatDistortionControl;
			params.engageTurbo = ssatTurbo;
			params.toneControl_010 = ssatToneControl;
			params.inputGainTweak_010 = ssatInputTrim;
			params.outputGain_010 = ssatVolumeControl;
			distortion.setParameters(params);
			//distortion.setSampleRate(sampleRate);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];
				//float sample = currinput[i];
				float outsample = (float)distortion.processAudioSample(sample);

				convertBuffer[i] = outsample;
			}
		}
							  break;
		case etModulatedDelay: {
			ModulatedDelayParameters params{};
			params.algorithm = (modDelaylgorithm)modDelayEffect;
			params.feedback_Pct = modDelayFeedback;
			params.lfoDepth_Pct = modDelayDepth;
			params.lfoRate_Hz = modDelayRate;
			modulatedDelay.setParameters(params);
			//modulatedDelay.setSampleRate(sampleRate);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];
				float outsample;
				modulatedDelay.processAudioFrame(&sample, &outsample, 1, 1);
				convertBuffer[i] = outsample;
			}
		}
							 break;
		case etStereoDelay: {
			AudioDelayParameters params{};
			params.algorithm = (delayAlgorithm)sdelAlgorithm;
			params.delayRatio_Pct = sdelRatio;
			params.dryLevel_dB = sdelDryLevel;
			params.wetLevel_dB = sdelWetLevel;
			params.feedback_Pct = sdelFeedback;
			params.leftDelay_mSec = params.rightDelay_mSec = sdelDelayTime;
			params.updateType = delayUpdateType::kLeftPlusRatio;
			audioDelay.setParameters(params);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];
				float outsample = (float) audioDelay.processAudioSample(sample);
				convertBuffer[i] = outsample;
			}

		}
						  break;
		case etReverb:
		{
			ReverbTankParameters params{};
			params.kRT = reverbParams.reverbTime;
			params.lpf_g = reverbParams.damping;
			params.apfDelayMax_mSec = reverbParams.apfDelayMax;
			params.apfDelayWeight_Pct = reverbParams.apfDelayWeight;
			params.density =(reverbDensity) reverbParams.density;
			params.fixeDelayMax_mSec = reverbParams.fixedDelayMax;
			params.fixeDelayWeight_Pct = reverbParams.fixedDelayWeight;
			params.dryLevel_dB = reverbParams.dryLevelDb;
			params.wetLevel_dB = reverbParams.wetLevelDb;
			params.highShelf_fc = reverbParams.hiShelfFc;
			params.highShelfBoostCut_dB = reverbParams.hiShelfGain;
			params.lowShelf_fc = reverbParams.lowShelfFc;
			params.lowShelfBoostCut_dB = reverbParams.lowShelfGain;
			reverbTank.setParameters(params);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];
				float outsample;
				reverbTank.processAudioFrame(&sample, &outsample, 1, 1);
				convertBuffer[i] = outsample;
			}

		}
		break;
		case etPreamp:
		{
			ClassATubePreParameters params{};
			params.highShelfBoostCut_dB = preampParams.hiShelfGain;
			params.highShelf_fc = preampParams.hiShelfFc;
			params.lowShelfBoostCut_dB = preampParams.lowShelfGain;
			params.lowShelf_fc = preampParams.lowShelfFc;
			params.saturation = preampParams.saturation;
			params.inputLevel_dB = preampParams.inputLevelDb;
			params.outputLevel_dB = preampParams.outputLevelDb;
			preamp.setParameters(params);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];
				float outsample =	(float)preamp.processAudioSample(sample);
				convertBuffer[i] = outsample;
			}
		}
			break;
		case etPhaser:
		{
			PhaseShifterParameters params{};
			params.intensity_Pct = phaserParams.intensity;
			params.lfoDepth_Pct = phaserParams.lfoDepth;
			params.lfoRate_Hz = phaserParams.lfoRate;
			phaseShifter.setParameters(params);
			for (int i = 0; i < samples; i++) {
				float sample = convertBuffer[i];
				float outsample = (float)phaseShifter.processAudioSample(sample);
				convertBuffer[i] = outsample;
			}
		}
		break;
		}
	}
	
	if(recording)
		waveFileData.insert(waveFileData.end(), convertBuffer.begin(), convertBuffer.end());
	if (applyMetronome) {
		auto curr = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( curr - lastTime);
		float millis = 60000.f / (float)metroBPM;//milliseconds per beat
		
		if (duration.count() > millis) {
			soundPlaying[0] = true;
			soundPos[0] = 0;
			lastTime = curr;
			
		}
		
		if (soundPlaying[0]) {//add sound
			int pos = soundPos[0];
			int maxpos = (int)sounds[0].size();
			int sndcnt = maxpos - pos;
			int count = samples < sndcnt ? samples : sndcnt;
			for (int i = 0; i < count; i++, pos++) {
				float sample = convertBuffer[i];
				float wavsample = sounds[0][pos];
				convertBuffer[i] = (sample + wavsample) * 0.5f;
			}
			if (pos >= maxpos)
				soundPlaying[0] = false;
			soundPos[0] = pos;
		}
	}
	memcpy(outputBuffer, convertBuffer.data(), samples*sizeof(float));
	if (outputCount > 1) {
		float* outputBuffer2 = outputBuffers[1];
		memcpy(outputBuffer2, convertBuffer.data(), samples * sizeof(float));
	}
	
#endif
	
}
void FXApp::BuildUniformBuffers(VulkContext& context) {
	{
		std::vector<UniformBufferInfo> bufferInfo;
		VkDeviceSize uboSize = sizeof(UBO);
		UniformBufferBuilder::begin(context.device, context.deviceProperties, context.memoryProperties, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true)
			.AddBuffer(uboSize, 1, 1)
			.build(uniformBuffer, bufferInfo);
		uniformBufferPtr = std::make_unique<VulkanUniformBuffer>(context.device, uniformBuffer, bufferInfo);
		pUBO = (UBO*)bufferInfo[0].ptr;
	}
	{
		std::vector<UniformBufferInfo> bufferInfo;
		VkDeviceSize storageSize = sizeof(Lights);
		UniformBufferBuilder::begin(context.device, context.deviceProperties, context.memoryProperties, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, true)
			.AddBuffer(storageSize, 1, 1)
			.build(storageBuffer, bufferInfo);
		storageBufferPtr = std::make_unique<VulkanUniformBuffer>(context.device, storageBuffer, bufferInfo);
		pLights = (Lights*)bufferInfo[0].ptr;
	}
}

void FXApp::BuildDescriptors(VulkContext& context) {
	DescriptorSetBuilder::begin(context.pPoolCache, context.pLayoutCache)
		.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
		.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(descriptorSet, descriptorLayout);
	VkDescriptorBufferInfo uniformInfo{};
	uniformInfo.buffer = uniformBuffer.buffer;
	uniformInfo.offset = 0;
	uniformInfo.range = sizeof(UBO);
	VkDescriptorBufferInfo storageInfo{};
	storageInfo.buffer = storageBuffer.buffer;
	storageInfo.offset = 0;
	storageInfo.range = sizeof(Lights);
	DescriptorSetUpdater::begin(context.pLayoutCache, descriptorLayout, descriptorSet)
		.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniformInfo)
		.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &storageInfo)
		.update();
}

void FXApp::BuildPipeline(VulkContext& context) {
	std::vector<VkPushConstantRange> pushConsts = { {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(PushConst)} };
	PipelineLayoutBuilder::begin(context.device)
		.AddDescriptorSetLayout(descriptorLayout)
		.AddPushConstants(pushConsts)
		.build(pipelineLayout);
	pipelineLayoutPtr = std::make_unique<VulkanPipelineLayout>(context.device, pipelineLayout);

	std::vector<Vulkan::ShaderModule> shaders;
	VkVertexInputBindingDescription vertexInputDescription;
	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
	ShaderProgramLoader::begin(context.device)
		.AddShaderPath("../Shaders/pbr-notex.vert.spv")
		.AddShaderPath("../Shaders/pbr-notex.frag.spv")
		.load(shaders, vertexInputDescription, vertexAttributeDescriptions);

	PipelineBuilder::begin(context.device, pipelineLayout, swapchain->getRenderPass(), shaders, vertexInputDescription, vertexAttributeDescriptions)
		//.setFrontFace(VK_FRONT_FACE_CLOCKWISE)
		.setCullMode(VK_CULL_MODE_BACK_BIT)
		//.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
		.setDepthTest(VK_TRUE)
		.build(pipeline);
	pipelinePtr = std::make_unique<VulkanPipeline>(context.device, pipeline);
	for (auto& shader : shaders) {
		Vulkan::cleanupShaderModule(context.device, shader.shaderModule);
	}
}


void FXApp::SDLEvent(const SDL_Event* event) {
	ui->ProcessSDLEvent(event);
}

void FXApp::OnExit() {

	vkDeviceWaitIdle(vulkState.getDevice());
}

void FXApp::CreateSwapchain() {
	VulkSwapchainFlags flags{};
	VkClearValue clearValues[2] = { { 1.f,1.f,1.f,1.f } ,{1.f,0.f} };
	flags.clearValues = clearValues;
	flags.clearValueCount = 2;
	flags.clientWidth = clientWidth;
	flags.clientHeight = clientHeight;
	flags.surfaceCaps = vulkState.getSurfaceCapabilities();

	flags.presentMode = VK_PRESENT_MODE_FIFO_KHR;

	swapchain->Create(vulkState.getDevice(), vulkState.getSurface(), vulkState.getGraphicsQueue(), vulkState.getPresentQueue(), vulkState.getPhysicalDeviceMemoryProperties(), flags);
}

void FXApp::Resize() {

	swapchain->Resize(clientWidth, clientHeight);
	ui->Resize(clientWidth, clientHeight);

}

float FXApp::drawCompressor(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Compressor");
	ui->Knob("Threshold", &compThresholdDB, -40, 0, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Knee Width", &compKneeWidthDB, 0.f, 20.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Ratio", &compRatio, 1.f, 20.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Attack", &compAttack, 1.f, 100.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Release", &compRelease, 1.f, 5000.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Output Gain", &compOutputGainDB, -20, 20, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	
	
	
	if (ui->ToggleButton("limit", &compLimitGate)) {

	}
	ImGui::SameLine();
	ImGui::Text("Limit/Gate");
	ImGui::SameLine();
	std::vector<const char*>modes = { "compress","expand" };
	if (ui->Combo("mode", modes, compMode)) {

	}
	
	if (ui->ToggleButton("softknee", &compSoftKnee)) {

	}
	ImGui::SameLine();
	ImGui::Text("Soft Knee");
	
	
	if (ui->ToggleButton("stereolink", &compStereoLink)) {
	
	}
	ImGui::SameLine();
	ImGui::Text("SteroLink");
	/*ImGui::SliderFloat("Det Gain", &compInputGainDB, -12.f, 20.f);
	ImGui::SliderFloat("Threshold", &compThresholdDB, -60.f, 0.f);
	ImGui::SliderFloat("Attack Time", &compAttack, 1.f, 300);
	ImGui::SliderFloat("Release Time", &compRelease, 20, 5000);
	ImGui::SliderFloat("Ratio", &compRatio, 1, 20);
	ImGui::SliderFloat("Output Gain", &compOutputGainDB, 0, 20);
	ImGui::SliderFloat("Knee Width", &compKneeWidthDB, 0, 20);
	std::vector<const char*>compTypes = { "COMP","LIMIT","EXPAND","GATE" };
	int compP = compProcessor;
	if (ui->Combo("Processor", compTypes, compProcessor)) {
		if (compP != compProcessor) {

		}
	}
	int compT = compTimeConst;
	std::vector<const char*> compTimeConsts = { "Digital","Analog" };
	if (ui->Combo("Time Constant", compTimeConsts, compTimeConst)) {
		if (compT != compTimeConst) {

		}
	}	*/
	
	yPos += ImGui::GetWindowHeight()+1;
	ImGui::End();
	return yPos;
}

float FXApp::drawSuperSaturation(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Super Saturation");
	ui->Knob("Input Trim", &ssatInputTrim, 0.f, 10.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Volume",&ssatVolumeControl,0.f,10.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Saturation",&ssatDistortionControl,0.f,10.f,0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Tone", &ssatToneControl, 0.f, 10.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ui->ToggleButton("Turbo", &ssatTurbo);
	ImGui::SameLine();
	ImGui::Text("Turbo");
	ImGui::SameLine();
	ui->ToggleButton("bypass", &ssatBypass);
	ImGui::SameLine();
	ImGui::Text("Bypass");
	yPos += ImGui::GetWindowHeight()+1;
	ImGui::End();
	return yPos;
}

float FXApp::drawModDelay(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250.f, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Mod Delay");
	ui->Knob("Rate", &modDelayRate, 0.02f, 20.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Depth", &modDelayDepth, 0.f, 100.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Feedback", &modDelayFeedback, 0.f, 100.f,0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	std::vector<const char*>effects = { "flanger","chorus","vibrato"};
	ImGui::SetNextItemWidth(150.f);
	ui->Combo("Effect", effects, modDelayEffect);
	yPos += ImGui::GetWindowHeight()+1;
	ImGui::End();
	return yPos;
}

float FXApp::drawStereoDelay(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250.f, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Stereo Delay");
	ui->Knob("Delay Time", &sdelDelayTime, 0.f, 2000.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Feedback", &sdelFeedback, 0.f, 100.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Ratio", &sdelRatio, 1.f, 100.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	std::vector<const char*>algos = { "normal","pingpong" };
	ImGui::SetNextItemWidth(150.f);
	ui->Combo("Algorithm", algos, sdelAlgorithm);
	ImGui::SameLine();
	ui->Knob("Wet Level",&sdelWetLevel, -60.f, 12.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Dry Level",&sdelDryLevel, -60.f, 12.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	yPos += ImGui::GetWindowHeight()+1;
	ImGui::End();
	return yPos;
}

float FXApp::drawReverb(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250.f, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Reverb");
	ui->Knob("Reverb Time", &reverbParams.reverbTime, 0.1f, 1.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Damping", &reverbParams.damping, 0.f, 1.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Pre-Delay", &reverbParams.preDelayTime, 0.f, 100.f, 0.5, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	auto densities = ReverbParams::getDensities();
	ui->Combo("Denstity", densities, reverbParams.density);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.f);
	int preset = (int)reverbParams.type;
	auto presets = ReverbParams::getPresets();
	ui->Combo("Room Type", presets, preset);
	if ((ReverbParams::PresetType)preset != reverbParams.type) {
		reverbParams.Preset((ReverbParams::PresetType)preset);
	}
	ImGui::SameLine();
	ui->Knob("Wet Level", &reverbParams.wetLevelDb, -60.f, 12.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Dry Level", &reverbParams.dryLevelDb, -60.f, 12.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);



	yPos += ImGui::GetWindowHeight() + 1;
	ImGui::End();
	return yPos;
}
float FXApp::drawPreamp(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250.f, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Preamp");
	ui->Knob("Input", &preampParams.inputLevelDb, -60.f, 12.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Drive", &preampParams.saturation, 1.f, 10.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("LoShelf Fc", &preampParams.lowShelfFc, 20.f, 2000.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("LoShelf Gain", &preampParams.lowShelfGain, -20.f, 20.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("HiShelf Fc", &preampParams.hiShelfFc, 1000.f, 5000.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("HiShelf Gain", &preampParams.hiShelfGain, -20.f, 20.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Output Level", &preampParams.outputLevelDb, -60.f, 12.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);

	yPos += ImGui::GetWindowHeight() + 1;
	ImGui::End();
	return yPos;
}

float FXApp::drawPhaser(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250.f, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Phaser");
	ui->Knob("Rate", &phaserParams.lfoRate, 0.02f, 20.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Depth", &phaserParams.lfoDepth, 0.f, 100.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	ImGui::SameLine();
	ui->Knob("Intensity", &phaserParams.intensity, 0.f, 100.f, 0.5f, nullptr, ImGuiKnobVariant_WiperDot);
	yPos += ImGui::GetWindowHeight() + 1;
	ImGui::End();
	return yPos;
}

float FXApp::drawMetronome(float yPos) {
	ImGui::SetNextWindowPos(ImVec2(250.f, yPos));
	ImGui::SetNextWindowSize(ImVec2(0, 0));
	ImGui::Begin("Metronome");
	ui->DragInt("BPM", &metroBPM, 1.5f, 40, 240);
	yPos += ImGui::GetWindowHeight() + 1;
	ImGui::End();
	return yPos;
}

void FXApp::queueEffect(EffectType type, bool add) {
	auto iter = std::find(effectsList.begin(), effectsList.end(), type);
	if (add) {
		if (iter == effectsList.end()) {
			effectsList.push_back(type);
		}
	}
	else {		
		if (iter != effectsList.end())
			effectsList.erase(iter);

	}
	threadEffects[effectsList.size()] = etNone;
}

void FXApp::Update(float delta) {

	swapchain->NextFrame();

	if (keys.find(SDL_SCANCODE_W) != keys.end())
		camera.ProcessKeyboard(FORWARD, delta);
	if (keys.find(SDL_SCANCODE_S) != keys.end())
		camera.ProcessKeyboard(BACKWARD, delta);
	if (keys.find(SDL_SCANCODE_A) != keys.end())
		camera.ProcessKeyboard(LEFT, delta);
	if (keys.find(SDL_SCANCODE_D) != keys.end())
		camera.ProcessKeyboard(RIGHT, delta);
	if (firstMouse) {
		lastX = (float)mouseInfo.pt.x;
		lastY = (float)mouseInfo.pt.y;
		firstMouse = false;
	}
	float xoffset = mouseInfo.pt.x - lastX;
	float yoffset = mouseInfo.pt.y - lastY;

	lastX = (float)mouseInfo.pt.x;
	lastY = (float)mouseInfo.pt.y;

	camera.ProcessMouseMovement(xoffset, yoffset);
	static float oldScroll = 0.f;
	if (oldScroll != 0.f && oldScroll != mouseInfo.scrollY) {
		pUBO->projection = glm::perspective(glm::radians(camera.Zoom), (float)clientWidth / (float)clientHeight, 0.1f, 100.f);
		pUBO->projection[1][1] *= -1;
		camera.ProcessMouseScroll(mouseInfo.scrollY);
	}
	oldScroll = mouseInfo.scrollY;

	pUBO->view = camera.GetViewMatrix();
	pUBO->camPos = camera.Position;

	if (keys.find(SDL_SCANCODE_I) != keys.end()) {
		showUI = !showUI;
	}
	//UI stuff
	if (showUI) {
		ui->Update();
		ui->NewFrame();

		//	ImGui::ShowDemoWindow();
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
		/*ImGui::SetNextWindowSize(ImVec2((float)clientWidth, 25.f));*/
		bool isOpen = false;

		if (ImGui::Begin("Menu", &isOpen, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar)) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Exit", "Alt+F4")) {
						if (isAsioStarted)
							asioMgr->Stop();
						Quit();
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Audio")) {

					ImGui::MenuItem("Preferences...", "Ctrl+P", &showASIOPopup);// []()->void {
						/*if (ImGui::BeginPopupModal("Asio Setup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

						}
					}
					);*/

					if (ImGui::MenuItem(isAsioStarted ? "Stop" : "Start", "Ctrl+S", &isAsioStarted, isAsioReady)) {
						if (isAsioStarted) {
							asioMgr->Start(audioCallback, this);

						}
						else {
							asioMgr->Stop();
						}
					}
					bool ctrlSel = false;
					if (ImGui::MenuItem("Control Panel", "Ctrl+I", &ctrlSel)) {
						ASIOControlPanel();
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();

			}
			ImGui::End();
		}
		if (showASIOPopup) {
			ImGui::OpenPopup("ASIO Settings");
			if (ImGui::BeginPopupModal("ASIO Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

				int sel = selectedDevice;
				ui->Combo("ASIO Device", asioDeviceList, sel);
				if (sel != selectedDevice) {
					setAsioDriver(sel);
				}
				ui->Text("Channels");
				sel = inputChannel;
				ui->Combo("Input", inputChannelNames, sel);
				if (sel != inputChannel) {
					inputChannel = sel;
				}
				int outc = outputChannels.size() > 0 ? outputChannels[0] : -1;
				sel = outc;
				ui->Combo("Output", outputChannelNames, sel);
				if (sel != outc) {
					outc = sel;
					if (outputChannels.size() > 0) {
						outputChannels[0] = outc;
					}
					else {
						outputChannels.push_back(outc);
					}
				}
				if (sel != -1) {
					outc = outputChannels.size() > 1 ? outputChannels[1] : -1;
					sel = outc;
					ui->Combo("Output 2", outputChannelNames, sel);
					if (sel != outc) {
						outc = sel;
						if (sel != outputChannels[0]) {
							if (outputChannels.size() > 1) {
								outputChannels[1] = outc;
							}
							else {
								outputChannels.push_back(outc);
							}
						}
					}
				}
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
					setAsioDeviceSettings(inputChannel, outputChannels);
					//asioMgr->InitBuffers(inputChannel, outputChannel);
					//asioMemBytes = asioMgr->GetMemBytes();
					//asioMgr->GetBuffers((void**)&inputBuffers, (void**)&outputBuffers);
					showASIOPopup = false;
					//isAsioReady = selectedDevice != -1 && inputChannel != -1 && outputChannel != -1;
				}
				ImGui::EndPopup();
			}
		}
		if (ui->BeginMainStatusBar()) {
			if (ImGui::BeginMenuBar()) {
				char buffer[256];
				char outcs[128];
				if (outputChannels.size() == 2) {
					sprintf_s(outcs, "%s,%s", outputChannelNames[outputChannels[0]], outputChannelNames[outputChannels[1]]);
				}
				else if(outputChannels.size()==1){
					sprintf_s(outcs, "%s", outputChannelNames[outputChannels[0]]);
				}
				else {
				strcpy_s(outcs, "(none)");
				}
				sprintf_s(buffer, "Device: %s, Input Channel: %s, Output Channel(s): %s, Sample Rate %d, Latency %d", selectedDevice == -1 ? "(None)" : asioDeviceList[selectedDevice], inputChannel == -1 ? "(None)" : inputChannelNames[inputChannel], outcs, (int)sampleRate, (int)(inputLatency + outputLatency) / 100);
				ImGui::Text(buffer);
				ImGui::EndMenuBar();
			}
			ImGui::End();
		}
		ImGui::SetNextWindowPos(ImVec2(0.f, 50.f));
		ImGui::SetNextWindowSize(ImVec2(0.f, 0.f));
		ui->BeginWindow("Amp", ImVec2(0.f, 50.f), ImVec2(250.f, 250.f));
		ui->Text("Power:");
		ui->SameLine();
		bool start = isAsioStarted;
		if (ui->ToggleButton("Power", &start)) {
			if (isAsioStarted == false && asioMgr->isLoaded()) {
				asioMgr->Start(audioCallback, this);
				isAsioStarted = true;
			}
		}
		else if (isAsioStarted) {
			isAsioStarted = false;

			asioMgr->Stop();
		}
			
			
		
		
		if (isAsioStarted) {
			windowYPos = 50.f;
			ui->Text("Metronome"); ui->SameLine();
			bool metro = applyMetronome;
			if (ui->ToggleButton("Metronome", &metro)) {

			}
			
			if (metro != applyMetronome) {
				
				/*if (soundPlaying[0] == false) {
					soundPos[0] = 0;
					soundPlaying[0] = true;
				}*/
				lastTime = std::chrono::high_resolution_clock::now();
				applyMetronome = metro;
			}
			bool pre = applyPreamp;
			ui->Text("Preamp"); ui->SameLine();
			if (ui->ToggleButton("Preamp", &pre)) {

			}
			if (pre != applyPreamp) {
				queueEffect(etPreamp, pre);
				applyPreamp = pre;
				if (!pre)
					preampParams.reset();
			}
			bool comp=applyCompression;
			ui->Text("Compressor"); ui->SameLine();
			if (ui->ToggleButton("Compression", &comp)&&comp)
			{
				
				/*ImGui::SliderFloat("Threshold", &fCompThreshold, -60.f, -20.f);
				ImGui::SliderFloat("Ratio", &fCompRatio, 1.f, 20.f);*/
				//windowYPos = drawCompressor(windowYPos);
			}

			if (comp != applyCompression) {
				queueEffect(etDynamics, comp);
				
				applyCompression = comp;
			}
			ui->Text("Super Saturation"); ui->SameLine();
			bool ssat = applySuperSat;
			if (ui->ToggleButton("ssat", &ssat)&&ssat) {
				
				//windowYPos = drawSuperSaturation(windowYPos);
			}
			if (ssat != applySuperSat) {
				queueEffect(etSuperSaturation, ssat);
				
				applySuperSat = ssat;
			}
			ui->Text("Mod Delay"); ui->SameLine();
			bool moddelay = applyModDelay;
			if (ui->ToggleButton("chorus", &moddelay)) {
				//windowYPos = drawModDelay(windowYPos);
			}
			
			if (moddelay != applyModDelay) {
				queueEffect(etModulatedDelay, moddelay);
				
				applyModDelay = moddelay;
			}
			ui->Text("Stero Delay"); ImGui::SameLine();
			bool stereoDelay = applyStereoDelay;
			if (ui->ToggleButton("sdelay", &stereoDelay)) {
				//windowYPos=drawStereoDelay(windowYPos);
			}
			if (stereoDelay != applyStereoDelay) {
				queueEffect(etStereoDelay, stereoDelay);
				applyStereoDelay = stereoDelay;
			}
			ui->Text("Reverb"); ImGui::SameLine();
			bool reverb = applyReverb;
			if (ui->ToggleButton("reverb", &reverb)) {
			}
			if (reverb != applyReverb) {
				queueEffect(etReverb, reverb);
				applyReverb = reverb;
			}
			ui->Text("Phaser"); ImGui::SameLine();
			bool phaser = applyPhaser;
			if (ui->ToggleButton("phaser", &phaser)) {

			}
			if (phaser != applyPhaser) {
				queueEffect(etPhaser, phaser);
				applyPhaser = phaser;
			}
			if (metro) {
				
				windowYPos = drawMetronome(windowYPos);
			}
			for (auto& fx : effectsList) {
				switch (fx) {
				case etDynamics:
					windowYPos = drawCompressor(windowYPos);
					break;
				case etSuperSaturation:
					windowYPos = drawSuperSaturation(windowYPos);
					break;
				case etModulatedDelay:
					windowYPos = drawModDelay(windowYPos);
					break;
				case etStereoDelay:
					windowYPos = drawStereoDelay(windowYPos);
					break;
				case etReverb:
					windowYPos = drawReverb(windowYPos);
					break;
				case etPreamp:
					windowYPos = drawPreamp(windowYPos);
					break;
				case etPhaser:
					windowYPos = drawPhaser(windowYPos);
					break;
				}
			}
			//bool delay = applyDelay;
			/*ui->Text("Delay"); ui->SameLine();

			if (ui->ToggleButton("Delay", &delay)) {
				ImGui::SliderInt("Samples", &delaySampleCount, 1, asioMemSize);

			}
			delayRegisters.resize(delaySampleCount);*/
			//we hold 
			//float vol = 0;
			//ImGui::SliderFloat("Volume", &fVol, -96, 0);
			ImGui::Text("Record:"); ImGui::SameLine();
			bool record = recording;
			if (ui->ToggleButton("record", &record)) {
				
			}
			if (record != recording) {
				recording = record;
				if (recording == false)
					saveWAVFile();
			}
		}


		//float fVol = 5.f;
		//ui->Knob("Volume", &fVol, 0.f, 1.f,0.5f,nullptr,ImGuiKnobVariant_Stepped,0.f,ImGuiKnobFlags_ValueTooltip,12);
		ui->EndWindow();


		/*ui->BeginWindow("ASIO Setup", ImVec2(0.f,100.f), ImVec2(300.f, 250.f));

		int sel = selectedDevice;
		ui->Combo("ASIO Device", asioDeviceList, sel);
		if (sel != selectedDevice) {
			setAsioDriver(sel);
		}
		sel = inputChannel;
		ui->Combo("Input Channel:", inputChannelNames, sel);
		if (sel != inputChannel) {
			inputChannel = sel;
		}
		sel = outputChannel;
		ui->Combo("Output Channel:", outputChannelNames, sel);
		if (sel != outputChannel) {
			outputChannel = sel;
		}
		ui->EndWindow();*/
	}
}

void FXApp::Draw(float delta) {
	VkCommandBuffer cmd = swapchain->BeginRender();
	VkViewport viewport = { 0.0f,0.0f,(float)clientWidth,(float)clientHeight,0.0f,1.0f };
	vkCmdSetViewport(cmd, 0, 1, &viewport);
	VkRect2D scissor = { {0,0},{(uint32_t)clientWidth,(uint32_t)clientHeight} };
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	glm::mat4 model = glm::mat4(1.f);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, offsets);
	PushConst pushConst;
	for (int m = 0; m < meshes.size(); m++) {
		Mesh& mesh = meshes[m];
		pushConst.model = model;
		for (int i = 0; i < mesh.count; i++) {
			Material& mat = materials[mesh.materialStart + i];
			pushConst.albedo = mat.albedo;
			pushConst.metallic = mat.metallic;
			pushConst.roughness = mat.roughness;
			vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConst), &pushConst);
			//pUBO->model = model;
			Primitive& primitive = primitives[mesh.primitiveStart + i];
			vkCmdDrawIndexed(cmd, primitive.indexCount, 1, primitive.indexStart, primitive.vertexStart, 0);
		}
	}
	//render UI last 
	if(showUI)ui->Render(cmd, clientWidth, clientHeight);
	swapchain->EndRender(cmd);
}

int main() {
	auto intalign = alignof(int);
	auto charalign = alignof(char);
	auto shortalign = alignof(short);
	auto longalign = alignof(long);
	auto intsize = sizeof(int);
	auto longsize = sizeof(int);
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	FXApp app;
	if (app.Initialize("FX App", SCR_WIDTH,SCR_HEIGHT)) {
		app.Run();
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;

}

