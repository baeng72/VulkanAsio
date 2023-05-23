#pragma once
#include <vector>
#include <set>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <fstream>
#include <array>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <numeric>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif
#define __USE__VMA__				//want VMA memory management
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR	
#endif
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define DEFAULT_SCREEN_WIDTH 800
#define DEFAULT_SCREEN_HEIGHT 600