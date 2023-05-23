#include "pch.h"

#include "SDLApp.h"
#include <cassert>

SDLApp::SDLApp() {
	title[0] = 0;
}

SDLApp::~SDLApp() {

}

bool SDLApp::InitMainWindow(const char* title, bool resizeable) {
	strcpy_s(this->title, title);
	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);
	u32 flags = SDL_WINDOW_VULKAN;
	if (resizeable)
		flags |= SDL_WINDOW_RESIZABLE;
	mainWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, clientWidth, clientHeight, flags);
	return true;
}


bool SDLApp::Initialize(const char* title, int32_t width, int32_t height, bool resizeable) {
	if (width != INT32_MAX && height != INT32_MAX) {
		clientWidth = width;
		clientHeight = height;
	}
	return InitMainWindow(title, resizeable);

}

void SDLApp::Run() {
	SDL_Event sdlEvent;
	bool run = true;
	bool isActive = false;
	i64 startTime = SDL_GetTicks64();
	while (run) {
		while (SDL_PollEvent(&sdlEvent)) {
			SDLEvent(&sdlEvent);
			switch (sdlEvent.type) {
			case SDL_QUIT:
				run = false;
				break;
			case SDL_KEYDOWN:
				if (sdlEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					sdlEvent.type = SDL_QUIT;
					SDL_PushEvent(&sdlEvent);
				}
				keys.insert(sdlEvent.key.keysym.scancode);// = true;
				break;
			case SDL_KEYUP:
				keys.erase(sdlEvent.key.keysym.scancode);//keys[sdlEvent.key.keysym.scancode] = false;
				break;
			case SDL_WINDOWEVENT:
				switch (sdlEvent.window.event) {
				case SDL_WINDOWEVENT_FOCUS_GAINED:
					isActive = true;
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					isActive = false;
					break;
				case SDL_WINDOWEVENT_RESIZED:
					clientWidth = sdlEvent.window.data1;
					clientHeight = sdlEvent.window.data2;
					Resize();
					break;
				}

				break;
			case SDL_MOUSEBUTTONDOWN:
				buttons.insert(sdlEvent.button.button);// buttons[sdlEvent.button.button] = true;
				mouseInfo.pt = Point(sdlEvent.button.x, sdlEvent.button.y);
				break;
			case SDL_MOUSEBUTTONUP:
				buttons.erase(sdlEvent.button.button);// buttons[sdlEvent.button.button] = false;
				mouseInfo.pt = Point(sdlEvent.button.x, sdlEvent.button.y);
				break;
			case SDL_MOUSEWHEEL:
				mouseInfo.scrollY = sdlEvent.wheel.preciseY;
				mouseInfo.scrollX = sdlEvent.wheel.preciseX;

				break;
			case SDL_MOUSEMOTION:
				mouseInfo.pt = Point(sdlEvent.button.x, sdlEvent.button.y);
				break;
			}




		}
		if (isActive) {

			i64 t = SDL_GetTicks64();
			i64 delta = t - startTime;
			float deltaTime = (delta) * 0.001f;
			CalculateFrameStats(t);
			startTime = t;

			Update(deltaTime);
			Draw(deltaTime);


		}
	}
	OnExit();
}

void SDLApp::CalculateFrameStats(i64 ticks) {

	static int frameCnt = 0;
	static float timeElapsed = 0.f;

	frameCnt++;
	//timeElapsed += deltaTime;
	float tickspersecond = ticks * 0.001f;

	//if ((tickspersecond - timeElapsed) >= 1.f) {

	//	float fps = (float)frameCnt;//fps = frameCnt/1
	//	float mspf = 1000.f / fps;
	//	char buffer[256];
	//	sprintf_s(buffer, sizeof(buffer), "%s    fps:%f,   mspf:%f", title, fps, mspf);
	//	SDL_SetWindowTitle(mainWindow, buffer);
	//	//Reset
	//	frameCnt = 0;
	//	timeElapsed += 1.f;
	//}
}

bool SDLApp::IsKeyDown(int32_t key) {
	bool keyDown = false;
	if (keys.find(key) != keys.end()) {
		keyDown = true;
		keys.erase(key);
	}
	return keyDown;
}

