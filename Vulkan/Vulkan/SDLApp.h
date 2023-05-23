#pragma once
#undef main
struct Point {
	i32	x;
	i32 y;

	Point() {
		x = y = 0;
	}
	Point(i32 x_, i32 y_) {
		x = x_;
		y = y_;
	}
};
class SDLApp {
protected:
	char title[128];
	SDL_Window* mainWindow;
	std::set<uint8_t> keys;
	std::set<uint8_t> buttons;
	struct MouseInfo {
		float scrollX;
		float scrollY;
		Point	pt;
	}mouseInfo;
	bool appPaused{ false };
	bool appMinimized{ false };
	bool appMaximized{ false };
	bool appFullScreen{ false };
	int32_t clientWidth{ DEFAULT_SCREEN_WIDTH };
	int32_t clientHeight{ DEFAULT_SCREEN_HEIGHT };
	bool InitMainWindow(const char* title, bool resizeable);
	void CalculateFrameStats(int64_t delta);
	virtual void Update(float delta) {};
	virtual void Draw(float delta) {};
	virtual void Resize() {}
	virtual void SDLEvent(const SDL_Event* event) {}
	virtual void OnExit() {};
	bool IsKeyDown(int32_t key);
public:
	SDLApp();
	~SDLApp();
	virtual bool Initialize(const char* title, int32_t width = INT32_MAX, int32_t height = INT32_MAX, bool resizeable = false);
	void Run();
	void Quit() {
		SDL_Event sdlEvent; 
		sdlEvent.type = SDL_QUIT;
		SDL_PushEvent(&sdlEvent);
	}
};

