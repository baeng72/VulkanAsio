#pragma once
#include <vector>
#include <cassert>
#include <asiodrivers.h>
#include <asio.h>
#include <atomic>
#include <mutex>
extern AsioDrivers* asioDrivers;
class AsioDriverMgr : public AsioDrivers {
public:
	AsioDriverMgr() {
		asioDrivers = this;
	}


};


typedef void (*audioCallback)(float** inputBuffers, int inputChannels, float** outputBuffers, int outputChannels,int samples,void*userPtr);
class AsioMgr {
	static ASIOTime* bufferSwitchTimeInfo(ASIOTime* time, long doubleBufferIndex, ASIOBool processNow);
	static void bufferSwitch(long doubleBufferIndex, ASIOBool directProcess);
	static void sampleRateDidChange(ASIOSampleRate rate);
	static long asioMessage(long selector, long value, void* message, double* opt);
	audioCallback pCallback;
	void* pUserPtr;
	ASIODriverInfo drvInfo{ 2 };
	long inputChannelCount;
	long outputChannelCount;
	ASIOSampleType inputType;
	ASIOSampleType outputType;
	std::vector<ASIOChannelInfo> channels;
	ASIOCallbacks callbacks;
	long memSize;
	ASIOSampleRate rate;
	std::vector<ASIOBufferInfo> channelBuffers;
	std::atomic<bool> loaded{ false };
	std::atomic<bool> isRunning{ false };
	bool postOutput{ false };
	std::atomic<bool> calledback;
	int xruns{ 0 };
	float** inputBuffers{ nullptr };
	float** outputBuffers{ nullptr };
	static void docallback(int index);
	int selectedInputChannelCount{ 0 };
	int selectedOutputChannelCount{ 0 };
	std::mutex lock;
public:
	AsioMgr(void* sysRef);
	~AsioMgr();
	int getNumInputChannels()const { return (int)inputChannelCount; }
	int getNumOutputChannels()const { return (int)outputChannelCount; }
	int getLatencies(long* pinput, long* poutput) { return ASIOGetLatencies(pinput, poutput); }
	const char* getInputChannelName(int idx)const { assert(idx >= 0 && idx < (int)inputChannelCount); return channels[idx].name; }
	const char* getOutputChannelName(int idx)const { assert(idx >= 0 && idx < (int)outputChannelCount); return channels[idx + (int)inputChannelCount].name; }
	ASIOSampleRate getSampleRate()const { return rate; }
	int GetTypeSize(ASIOSampleType type)const;
	int GetMemSize()const { return memSize; }
	int GetMemBytes()const { return memSize*GetTypeSize(inputType); }
	bool InitBuffers(std::vector<long>& inputChannels, std::vector<long>& outputChannels);
	void FreeBuffers();	
	bool Start(audioCallback pC, void* up);
	bool Stop();
	size_t GetChannelBufferCount()const { return channelBuffers.size(); }
	bool isLoaded()const { return loaded; }
	

};