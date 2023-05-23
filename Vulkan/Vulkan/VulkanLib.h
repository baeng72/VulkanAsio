#pragma once

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <vector>
#include <cassert>
#include <cmath>
#include "defines.h"
#include "types.h"
//#define VK_USE_PLATFORM_WIN32_KHR
//#include <algorithm>
#include <vulkan/vulkan.h>
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <SDL2/SDL.h>
#include <sdl2/sdl_vulkan.h>
#include <SDL2/SDL_syswm.h>
#include "Vulkan.h"
#include "VulkanEx.h"
//#define ENABLE_DEBUG_MARKER		//define to enable debugging for RenderDoc
#include "VulkanDebug.h"
#include "VulkState.h"
#include "TextureLoader.h"
#include "SDLApp.h"

