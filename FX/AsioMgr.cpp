#include "AsioMgr.h"
#include <chrono>
using namespace std::chrono;
AsioMgr* pThis = nullptr;
AsioMgr::AsioMgr(void* sysRef) {
	pThis = this;
	pCallback = nullptr;
	pUserPtr = nullptr;
	loaded = false;
	calledback = false;
	inputChannelCount = outputChannelCount = 0;
	memSize = 0;// inputMemBytes = outputMemBytes = 0;
	rate = 48000;
	drvInfo.sysRef = sysRef;
	drvInfo.asioVersion = 2;
	ASIOError err = ASIOInit(&drvInfo);
	assert(ASE_OK == err);
	if (ASE_OK == err) {
		err = ASIOGetChannels(&inputChannelCount, &outputChannelCount);
		assert(ASE_OK == err);
		
		long numChannels = inputChannelCount + outputChannelCount;
		channels.resize(numChannels);
		for (long l = 0; l < numChannels; l++) {
			ASIOBool isInput = l < inputChannelCount;
			channels[l].isInput = isInput;
			channels[l].channel = l < inputChannelCount ? l : l - inputChannelCount;
			err = ASIOGetChannelInfo(&channels[l]);
			assert(ASE_OK == err);			
		}
		if (ASIOFuture(kAsioCanReportOverload, nullptr) != ASE_OK)
			xruns = -1;
		ASIOSampleRate currRate;
		err = ASIOGetSampleRate(&currRate);
		if (ASE_OK == err) {
			if (currRate != rate) {
				err = ASIOCanSampleRate(rate);
				if (ASE_OK == err) {
					err = ASIOSetSampleRate(rate);
					if (ASE_OK == err) {
						
					}
					else {
						err = ASIOGetSampleRate(&currRate);
						if (ASE_OK == err)
							rate = currRate;
					}
				}
			}
		}

		if (ASE_OK == err) {
			long minSize = 0, maxSize = 0, preferredSize = 0, granularity = 0;
			err = ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
			assert(ASE_OK == err);
			if (ASE_OK == err) {
				if (minSize == maxSize) {
					memSize = minSize;
				}
				else {
					memSize = preferredSize;
				}				
			}
		}
	}
}
AsioMgr::~AsioMgr() {
	
	Stop();

	if (inputBuffers) {
		for (int i = 0; i < selectedInputChannelCount; i++)
			delete[]inputBuffers[i];
		delete inputBuffers;
	}
	inputBuffers = nullptr;
	if (outputBuffers) {
		for (int i = 0; i < selectedOutputChannelCount; i++) {
			delete[]outputBuffers[i];
		}
		delete outputBuffers;
	}
	outputBuffers = nullptr;

}
int AsioMgr::GetTypeSize(ASIOSampleType type) const{
	int sampleSize = 0;
	switch (type) {
	case ASIOSTInt16MSB:
		sampleSize = 2;
		break;
	case ASIOSTInt24MSB:		// used for 20 bits as well
		sampleSize = 3;
		break;
	case ASIOSTInt32MSB:

	case ASIOSTFloat32MSB:		// IEEE 754 32 bit float
		sampleSize = 4;
		break;
	case	ASIOSTFloat64MSB:		// IEEE 754 64 bit double float
		sampleSize = 8;
		break;
	case ASIOSTInt32MSB16:		// 32 bit data with 16 bit alignment
	case ASIOSTInt32MSB18:		// 32 bit data with 18 bit alignment
	case ASIOSTInt32MSB20:		// 32 bit data with 20 bit alignment
	case ASIOSTInt32MSB24:		// 32 bit data with 24 bit alignment
		sampleSize = 4;
		break;
	case ASIOSTInt16LSB:
		sampleSize = 2;
		break;
	case ASIOSTInt24LSB:		// used for 20 bits as well
		sampleSize = 3;
		break;
	case ASIOSTInt32LSB:

	case ASIOSTFloat32LSB:		// IEEE 754 32 bit float, as found on Intel x86 architecture
		sampleSize = 4;
		break;
	case ASIOSTFloat64LSB: 		// IEEE 754 64 bit double float, as found on Intel x86 architecture
		sampleSize = 8;
		break;
	case ASIOSTInt32LSB16:		// 32 bit data with 18 bit alignment
	case ASIOSTInt32LSB18:		// 32 bit data with 18 bit alignment
	case ASIOSTInt32LSB20:		// 32 bit data with 20 bit alignment
	case ASIOSTInt32LSB24:		// 32 bit data with 24 bit alignment
		sampleSize = 8;
		break;
	default:

		sampleSize = 0;
		break;

	}
	return sampleSize;
}

bool AsioMgr::InitBuffers(std::vector<long>&inputChannels,std::vector<long>&outputChannels) {
	size_t inputChannelSize = inputChannels.size();
	size_t outputChannelSize = outputChannels.size();
	size_t totalChannels = inputChannelSize + outputChannelSize;
	channelBuffers.resize(totalChannels);
	for (size_t i = 0; i < totalChannels; i++) {
		channelBuffers[i].channelNum = i < inputChannelSize ? inputChannels[i] : outputChannels[i - inputChannelSize];
		channelBuffers[i].isInput = i < inputChannelSize ? ASIOTrue : ASIOFalse;
		channelBuffers[i].buffers[0] = channelBuffers[i].buffers[1] = nullptr;
	}	
	callbacks = {
							bufferSwitch,
							sampleRateDidChange,
							asioMessage,
							bufferSwitchTimeInfo
	};
	ASIOError err = ASIOCreateBuffers(channelBuffers.data(), (long)totalChannels, memSize, &callbacks);
	//assert(ASE_OK == err);
	if (ASE_OK == err) {
		inputBuffers = new float* [inputChannelSize];
		for (int i = 0; i < inputChannelSize; i++) {
			inputBuffers[i] = new float[memSize];
		}
		outputBuffers = new float* [outputChannelSize];
		for (int i = 0; i < outputChannelSize; i++) {
			outputBuffers[i] = new float[memSize];
		}
		std::vector<ASIOChannelInfo> infos(totalChannels);
		for (size_t i = 0; i < totalChannels; i++) {
			infos[i].channel = i < inputChannelSize ? inputChannels[i] : outputChannels[i - inputChannelSize];
			infos[i].isInput = i < inputChannelSize ? ASIOTrue : ASIOFalse;			
			if (ASE_OK == ASIOGetChannelInfo(&infos[i])) {
				if (i < inputChannelSize) {
					inputType = infos[i].type;
				}
				else {
					outputType = infos[i].type;
				}
				ASIOSampleRate rate;
				if (ASE_OK == ASIOGetSampleRate(&rate)) {

				}
			}
		}
		selectedInputChannelCount = inputChannelSize;
		selectedOutputChannelCount = outputChannelSize;		
		loaded = true;
		isRunning = false;
		err = ASIOOutputReady();
		postOutput = (ASE_OK == err);
	}
	return ASE_OK == err;
}
bool AsioMgr::Start(audioCallback pC,void*up) {
	pCallback = pC;
	pUserPtr = up;
	if (loaded){
		if (!isRunning) {
			if (ASE_OK == ASIOStart()) {
				int count = 300;
				while (--count > 0 && !calledback) {
					::Sleep(10);
				}
				return (isRunning = true);
			}
		}
	}
	return false;
}

bool AsioMgr::Stop() {
	if (isRunning) {
		if (ASE_OK == ASIOStop()) {
			isRunning = false;
			return true;
		}
	}
	return false;
}
void AsioMgr::FreeBuffers() {
	if (isRunning)
		Stop();
	if (loaded) {
		ASIODisposeBuffers();
		loaded = false;
		if (inputBuffers) {
			for (int i = 0; i < selectedInputChannelCount; i++)
				delete[]inputBuffers[i];
			delete inputBuffers;
		}
		inputBuffers = nullptr;
		if (outputBuffers) {
			for (int i = 0; i < selectedOutputChannelCount; i++) {
				delete[]outputBuffers[i];
			}
			delete outputBuffers;
		}
		outputBuffers = nullptr;
	}
}
ASIOTime* AsioMgr::bufferSwitchTimeInfo(ASIOTime* time, long doubleBufferIndex, ASIOBool processNow) {
	docallback(doubleBufferIndex);
	return time;
}
void AsioMgr::bufferSwitch(long doubleBufferIndex,//current buffer 0 or 1 returned by create buffer
	ASIOBool directProcess) {
	docallback(doubleBufferIndex);
	
}
void FromInt32ToFloat(int32_t* pinput, float* poutput, int samples) {
	float invdiv = 1.f / (float)INT32_MAX;
	for (int i = 0; i < samples; i++) {
		int32_t sample = pinput[i];
		float fsample = (float)(sample * invdiv);
		poutput[i] = fsample;
	}
}
void FromFloatToInt32(float* pinput, int32_t* poutput, int samples) {
	float mul = (float)INT32_MAX;
	for (int i = 0; i < samples; i++) {
		float fsample = pinput[i];
		int32_t sample = (int32_t)(fsample * mul);
		poutput[i] = sample;
	}
}
void AsioMgr::docallback(int index) {
	std::scoped_lock lock(pThis->lock);
	auto start = high_resolution_clock::now();
	auto samples = pThis->memSize;
	int inputCount = pThis->selectedInputChannelCount;
	int outputCount = pThis->selectedOutputChannelCount;
	if (pThis->isRunning) {
		if (pThis->pCallback) {
			float** ppinput = pThis->inputBuffers;
			float** ppoutput = pThis->outputBuffers;
			
			
			for (int i = 0; i < inputCount; i++) {
				int32_t* pinput = (int32_t*)pThis->channelBuffers[i].buffers[index];
				
				FromInt32ToFloat(pinput,ppinput[i],samples);
			}
			pThis->pCallback(ppinput, inputCount, ppoutput, outputCount, samples, pThis->pUserPtr);
			for (int i = 0; i < outputCount; i++) {
				int32_t* poutput = (int32_t*)pThis->channelBuffers[i + inputCount].buffers[index];
				FromFloatToInt32(ppoutput[i],poutput,samples);
			}
			ASIOOutputReady();
		}		
	}
	else {
		for (int i = 0; i < outputCount; i++) {
			int32_t* poutput = (int32_t*)pThis->channelBuffers[i + inputCount].buffers[index];
			memset(poutput, 0, samples * sizeof(int32_t));
		}
		ASIOOutputReady();
	}
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	if (duration.count() > 100) {
		int z = 0;
	}
	pThis->calledback = true;
}
void AsioMgr::sampleRateDidChange(ASIOSampleRate rate) {
	pThis->rate = rate;
}
long AsioMgr::asioMessage(long selector, long value, void* message, double* opt) {
	switch (selector) {
	case kAsioSelectorSupported:
		switch (value) {
		case kAsioSelectorSupported:
		case kAsioEngineVersion:
		case kAsioResetRequest:
		case kAsioBufferSizeChange:
		case kAsioResyncRequest:
		case kAsioLatenciesChanged:
		case kAsioSupportsTimeInfo:
		case kAsioSupportsTimeCode:
			return 1L;
		}
		break;
	case kAsioEngineVersion:
		return 2L;
	case kAsioResetRequest:
		return 1L;
	case kAsioBufferSizeChange:
	{
		long newBufferSize = value;
	}
	return 1L;
	case kAsioResyncRequest:
		return 1L;
	case kAsioLatenciesChanged:
		return 1L;
	case kAsioSupportsTimeInfo:
		return 1L;//support bufferSwitchTimeInfo()
	case kAsioSupportsTimeCode:
		return 1;
	}
	return 0;//undefined selectors return 0
}
