#include "ui.h"
#include "imgui_internal.h"
UI::UI() {

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
}

UI::~UI() {
	vkDeviceWaitIdle(context.device);
	pipelinePtr.reset();
	pipelineLayoutPtr.reset();
	descriptorPtr.reset();
	indexBufferPtr.reset();
	vertexBufferPtr.reset();
	fontTexturePtr.reset();
	ImGui::DestroyContext();
}

void UI::Init(VulkContext& context, VkRenderPass renderPass,const char *shaderPath) {
	this->context = context;
	ImGuiIO& io = ImGui::GetIO();
	//create font
	{

		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		size_t upload_size = width * height * 4 * sizeof(char);
		fontTexture = TextureBuilder::begin(context.device, context.memoryProperties)
			.setDimensions(width, height)
			.setFormat(PREFERRED_IMAGE_FORMAT)
			.setImageUsage(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.setImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
			.build();
		VkDeviceSize stagingSize = upload_size;
		Vulkan::Buffer stagingBuffer = StagingBufferBuilder::begin(context.device, context.memoryProperties)
			.setSize(stagingSize)
			.build();
		u8* ptr = (u8*)Vulkan::mapBuffer(context.device, stagingBuffer);
		memcpy(ptr, pixels, upload_size);
		Vulkan::transitionImage(context.device, context.queue, context.commandBuffer, fontTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		Vulkan::CopyBufferToImage(context.device, context.queue, context.commandBuffer, stagingBuffer, fontTexture, width, height);
		Vulkan::unmapBuffer(context.device, stagingBuffer);
		Vulkan::cleanupBuffer(context.device, stagingBuffer);
		Vulkan::transitionImage(context.device, context.queue, context.commandBuffer, fontTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		fontTexturePtr = std::make_unique<VulkanTexture>(context.device, fontTexture);

	}
	DescriptorSetBuilder::begin(context.pPoolCache, context.pLayoutCache)
		.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(descriptor, descriptorLayout);
	descriptorPtr = std::make_unique<VulkanDescriptor>(context.device, descriptor);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = fontTexture.imageView;
	imageInfo.sampler = fontTexture.sampler;

	DescriptorSetUpdater::begin(context.pLayoutCache, descriptorLayout, descriptor)
		.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo)
		.update();

	std::vector<VkPushConstantRange> pushContants{ {VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(IGPushConstants)} };

	PipelineLayoutBuilder::begin(context.device)
		.AddDescriptorSetLayout(descriptorLayout)
		.AddPushConstants(pushContants)
		.build(pipelineLayout);
	pipelineLayoutPtr = std::make_unique<VulkanPipelineLayout>(context.device, pipelineLayout);
	char vertPath[256];
	char fragPath[256];
	if (shaderPath) {
		strcpy_s(vertPath, shaderPath);
		strcpy_s(fragPath, shaderPath);

	}
	else {
		strcpy_s(vertPath, "../../Shaders");
		strcpy_s(fragPath, "../../Shaders");
	}
	strcat_s(vertPath, "/ImGui.vert.spv");
	strcat_s(fragPath, "/ImGui.frag.spv");
	std::vector<Vulkan::ShaderModule> shaders;
	VkVertexInputBindingDescription vertexInputDescription = {};
	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
	ShaderProgramLoader::begin(context.device)
		.AddShaderPath(vertPath)
		.AddShaderPath(fragPath)
		.load(shaders, vertexInputDescription, vertexAttributeDescriptions);
	//we pass a 32 bit uint for color, not a vec4, so update attribute description for color
	vertexAttributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertexInputDescription.stride = sizeof(float) * 4 + sizeof(u32);
	PipelineBuilder::begin(context.device, pipelineLayout, renderPass, shaders, vertexInputDescription, vertexAttributeDescriptions)
		.setCullMode(VK_CULL_MODE_FRONT_BIT)
		//.setDepthTest(VK_TRUE)
		.setBlend(VK_TRUE)

		.build(pipeline);
	pipelinePtr = std::make_unique<VulkanPipeline>(context.device, pipeline);

	for (auto& shader : shaders) {
		Vulkan::cleanupShaderModule(context.device, shader.shaderModule);
	}
}
void UI::Resize(u32 width, u32 height) {

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)width, (float)height);
	io.DisplayFramebufferScale = ImVec2(1, 1);
	//if (width > 0 && height > 0)
	//	io.DisplayFramebufferScale = ImVec2((float)width, (float)height);

}

void UI::Update() {
	static u64 frequency = SDL_GetPerformanceFrequency();
	u64 currentTime = SDL_GetPerformanceCounter();
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = time > 0 ? (float)((double)(currentTime - time) / frequency) : (float)(1.f / 60.f);
	time = currentTime;

}

void UI::Render(VkCommandBuffer cmd, u32 width, u32 height) {
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (!(draw_data->DisplaySize.x <= 0.f || draw_data->DisplaySize.y < 0.f)) {
		i32 fb_width = (i32)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
		i32 fb_height = (i32)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
		if (fb_width < 0 || fb_height < 0)
			return;
		if (draw_data->TotalVtxCount > 0) {
			//create or resize vertex/index buffers
			VkDeviceSize vertSize = draw_data->TotalVtxCount * sizeof(ImDrawVert);
			VkDeviceSize indSize = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
			//if(vertexBufferSize!=vertSize || indexBufferSize != indSize)

			if (vertexBufferSize < vertSize || indexBufferSize < indSize) {
				//vkQueueWaitIdle(context.queue);//hack!
				if (vertexPtr)
					Vulkan::unmapBuffer(context.device, vertexBuffer);
				if (vertexBufferPtr)
					vertexBufferPtr.reset();
				std::vector<u32> vertexLocations;
				if (vertSize < 1024 * 256)
					vertSize = 1024 * 256;
				if (indSize < 1024 * 128)
					indSize = 1024 * 128;
				if (vertexBufferSize > 0) {
					vkQueueWaitIdle(context.queue);
				}
				VertexBufferBuilder::begin(context.device, context.queue, context.commandBuffer, context.memoryProperties)
					.AddVertices(vertSize, nullptr, true)
					.build(vertexBuffer, vertexLocations, (void**)&vertexPtr);
				vertexBufferPtr = std::make_unique<VulkanVIBuffer>(context.device, vertexBuffer, vertexLocations);
				vertexBufferSize = vertSize;
				//vertexPtr = (u8*)Vulkan::mapBuffer(context.device, vertexBuffer);
				if (indexPtr)
					Vulkan::unmapBuffer(context.device, indexBuffer);
				if (indexBufferPtr)
					indexBufferPtr.reset();
				std::vector<u32> indexLocations;
				IndexBufferBuilder::begin(context.device, context.queue, context.commandBuffer, context.memoryProperties)
					.AddIndices(indSize, nullptr, true)
					.build(indexBuffer, indexLocations, (void**)&indexPtr);
				indexBufferPtr = std::make_unique<VulkanVIBuffer>(context.device, indexBuffer, indexLocations);
				indexBufferSize = indSize;
				//indexPtr = (u8*)Vulkan::mapBuffer(context.device, indexBuffer);

			}
			ImDrawVert* vptr = (ImDrawVert*)vertexPtr;
			ImDrawIdx* iptr = (ImDrawIdx*)indexPtr;
			//upload vertex/index data


			for (i32 n = 0; n < draw_data->CmdListsCount; n++) {
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				memcpy(vptr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(iptr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

				vptr += cmd_list->VtxBuffer.Size;
				iptr += cmd_list->IdxBuffer.Size;

			}
			Vulkan::flushBuffer(context.device, vertexBuffer, vertexBufferSize);
			Vulkan::flushBuffer(context.device, indexBuffer, indexBufferSize);
		}
		

		if (vertexBufferSize > 0) {
			// Will project scissor/clipping rectangles into framebuffer space
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
			// Setup viewport:
			{
				VkViewport viewport;
				viewport.x = 0;
				viewport.y = 0;
				viewport.width = (float)fb_width;
				viewport.height = (float)fb_height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(cmd, 0, 1, &viewport);
			}
			// Setup scale and translation:
	   // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
			{
				pushConst.scale.x = 2.0f / draw_data->DisplaySize.x;
				pushConst.scale.y = 2.0f / draw_data->DisplaySize.y;
				pushConst.translate.x = -1.0f - draw_data->DisplayPos.x * pushConst.scale.x;
				pushConst.translate.y = -1.0f - draw_data->DisplayPos.y * pushConst.scale.y;
				vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(IGPushConstants), &pushConst);

			}
			ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
			ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
			 // Render command lists
			// (Because we merged all buffers into a single one, we maintain our own offset into them)
			i32 global_vtx_offset = 0;
			i32 global_idx_offset = 0;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor, 0, nullptr);
			for (i32 n = 0; n < draw_data->CmdListsCount; n++) {
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
					// Project scissor/clipping rectangles into framebuffer space
					ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
					ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
					// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
					if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
					if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
					if (clip_max.x > width) { clip_max.x = (float)fb_width; }
					if (clip_max.y > height) { clip_max.y = (float)fb_height; }
					if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
						continue;
					// Apply scissor/clipping rectangle
					VkRect2D scissor;
					scissor.offset.x = (int32_t)(clip_min.x);
					scissor.offset.y = (int32_t)(clip_min.y);
					scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
					scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
					vkCmdSetScissor(cmd, 0, 1, &scissor);


					vkCmdDrawIndexed(cmd, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
				}
				global_idx_offset += cmd_list->IdxBuffer.Size;
				global_vtx_offset += cmd_list->VtxBuffer.Size;
			}
			//Vulkan::flushBuffer(context.device, vertexBuffer, global_vtx_offset);
			//Vulkan::flushBuffer(context.device, indexBuffer, global_idx_offset);
			// Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
	  //// Our last values will leak into user/application rendering IF:
	  // - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
	  // - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitly set that state.
	  // If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
	  // In theory we should aim to backup/restore those values but I am not sure this is possible.
	  // We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
			VkRect2D scissor = { { 0, 0 }, { (uint32_t)fb_width, (uint32_t)fb_height } };
			vkCmdSetScissor(cmd, 0, 1, &scissor);
		}
	}
}

ImGuiKey UI::KeycodeToImGuiKey(i32 keycode) {
	switch (keycode)
	{
	case SDLK_TAB: return ImGuiKey_Tab;
	case SDLK_LEFT: return ImGuiKey_LeftArrow;
	case SDLK_RIGHT: return ImGuiKey_RightArrow;
	case SDLK_UP: return ImGuiKey_UpArrow;
	case SDLK_DOWN: return ImGuiKey_DownArrow;
	case SDLK_PAGEUP: return ImGuiKey_PageUp;
	case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
	case SDLK_HOME: return ImGuiKey_Home;
	case SDLK_END: return ImGuiKey_End;
	case SDLK_INSERT: return ImGuiKey_Insert;
	case SDLK_DELETE: return ImGuiKey_Delete;
	case SDLK_BACKSPACE: return ImGuiKey_Backspace;
	case SDLK_SPACE: return ImGuiKey_Space;
	case SDLK_RETURN: return ImGuiKey_Enter;
	case SDLK_ESCAPE: return ImGuiKey_Escape;
	case SDLK_QUOTE: return ImGuiKey_Apostrophe;
	case SDLK_COMMA: return ImGuiKey_Comma;
	case SDLK_MINUS: return ImGuiKey_Minus;
	case SDLK_PERIOD: return ImGuiKey_Period;
	case SDLK_SLASH: return ImGuiKey_Slash;
	case SDLK_SEMICOLON: return ImGuiKey_Semicolon;
	case SDLK_EQUALS: return ImGuiKey_Equal;
	case SDLK_LEFTBRACKET: return ImGuiKey_LeftBracket;
	case SDLK_BACKSLASH: return ImGuiKey_Backslash;
	case SDLK_RIGHTBRACKET: return ImGuiKey_RightBracket;
	case SDLK_BACKQUOTE: return ImGuiKey_GraveAccent;
	case SDLK_CAPSLOCK: return ImGuiKey_CapsLock;
	case SDLK_SCROLLLOCK: return ImGuiKey_ScrollLock;
	case SDLK_NUMLOCKCLEAR: return ImGuiKey_NumLock;
	case SDLK_PRINTSCREEN: return ImGuiKey_PrintScreen;
	case SDLK_PAUSE: return ImGuiKey_Pause;
	case SDLK_KP_0: return ImGuiKey_Keypad0;
	case SDLK_KP_1: return ImGuiKey_Keypad1;
	case SDLK_KP_2: return ImGuiKey_Keypad2;
	case SDLK_KP_3: return ImGuiKey_Keypad3;
	case SDLK_KP_4: return ImGuiKey_Keypad4;
	case SDLK_KP_5: return ImGuiKey_Keypad5;
	case SDLK_KP_6: return ImGuiKey_Keypad6;
	case SDLK_KP_7: return ImGuiKey_Keypad7;
	case SDLK_KP_8: return ImGuiKey_Keypad8;
	case SDLK_KP_9: return ImGuiKey_Keypad9;
	case SDLK_KP_PERIOD: return ImGuiKey_KeypadDecimal;
	case SDLK_KP_DIVIDE: return ImGuiKey_KeypadDivide;
	case SDLK_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case SDLK_KP_MINUS: return ImGuiKey_KeypadSubtract;
	case SDLK_KP_PLUS: return ImGuiKey_KeypadAdd;
	case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
	case SDLK_KP_EQUALS: return ImGuiKey_KeypadEqual;
	case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
	case SDLK_LSHIFT: return ImGuiKey_LeftShift;
	case SDLK_LALT: return ImGuiKey_LeftAlt;
	case SDLK_LGUI: return ImGuiKey_LeftSuper;
	case SDLK_RCTRL: return ImGuiKey_RightCtrl;
	case SDLK_RSHIFT: return ImGuiKey_RightShift;
	case SDLK_RALT: return ImGuiKey_RightAlt;
	case SDLK_RGUI: return ImGuiKey_RightSuper;
	case SDLK_APPLICATION: return ImGuiKey_Menu;
	case SDLK_0: return ImGuiKey_0;
	case SDLK_1: return ImGuiKey_1;
	case SDLK_2: return ImGuiKey_2;
	case SDLK_3: return ImGuiKey_3;
	case SDLK_4: return ImGuiKey_4;
	case SDLK_5: return ImGuiKey_5;
	case SDLK_6: return ImGuiKey_6;
	case SDLK_7: return ImGuiKey_7;
	case SDLK_8: return ImGuiKey_8;
	case SDLK_9: return ImGuiKey_9;
	case SDLK_a: return ImGuiKey_A;
	case SDLK_b: return ImGuiKey_B;
	case SDLK_c: return ImGuiKey_C;
	case SDLK_d: return ImGuiKey_D;
	case SDLK_e: return ImGuiKey_E;
	case SDLK_f: return ImGuiKey_F;
	case SDLK_g: return ImGuiKey_G;
	case SDLK_h: return ImGuiKey_H;
	case SDLK_i: return ImGuiKey_I;
	case SDLK_j: return ImGuiKey_J;
	case SDLK_k: return ImGuiKey_K;
	case SDLK_l: return ImGuiKey_L;
	case SDLK_m: return ImGuiKey_M;
	case SDLK_n: return ImGuiKey_N;
	case SDLK_o: return ImGuiKey_O;
	case SDLK_p: return ImGuiKey_P;
	case SDLK_q: return ImGuiKey_Q;
	case SDLK_r: return ImGuiKey_R;
	case SDLK_s: return ImGuiKey_S;
	case SDLK_t: return ImGuiKey_T;
	case SDLK_u: return ImGuiKey_U;
	case SDLK_v: return ImGuiKey_V;
	case SDLK_w: return ImGuiKey_W;
	case SDLK_x: return ImGuiKey_X;
	case SDLK_y: return ImGuiKey_Y;
	case SDLK_z: return ImGuiKey_Z;
	case SDLK_F1: return ImGuiKey_F1;
	case SDLK_F2: return ImGuiKey_F2;
	case SDLK_F3: return ImGuiKey_F3;
	case SDLK_F4: return ImGuiKey_F4;
	case SDLK_F5: return ImGuiKey_F5;
	case SDLK_F6: return ImGuiKey_F6;
	case SDLK_F7: return ImGuiKey_F7;
	case SDLK_F8: return ImGuiKey_F8;
	case SDLK_F9: return ImGuiKey_F9;
	case SDLK_F10: return ImGuiKey_F10;
	case SDLK_F11: return ImGuiKey_F11;
	case SDLK_F12: return ImGuiKey_F12;
	}
	return ImGuiKey_None;
}

bool UI::ProcessSDLEvent(const SDL_Event* event) {
	ImGuiIO& io = ImGui::GetIO();
	switch (event->type) {
	case SDL_MOUSEMOTION:
		io.AddMousePosEvent((float)event->motion.x, (float)event->motion.y);
		return true;
	case SDL_MOUSEWHEEL:
	{
		float wheel_x = (event->wheel.x > 0) ? 1.f : (event->wheel.x < 0) ? -1.f : 0.f;
		float wheel_y = (event->wheel.y > 0) ? 1.f : (event->wheel.y < 0) ? -1.f : 0.f;
		io.AddMouseWheelEvent(wheel_x, wheel_y);
		return true;
	}
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	{
		int mouse_button = -1;
		if (event->button.button == SDL_BUTTON_LEFT) { mouse_button = 0; }
		if (event->button.button == SDL_BUTTON_RIGHT) { mouse_button = 1; }
		if (event->button.button == SDL_BUTTON_MIDDLE) { mouse_button = 2; }
		if (event->button.button == SDL_BUTTON_X1) { mouse_button = 3; }
		if (event->button.button == SDL_BUTTON_X2) { mouse_button = 4; }
		if (mouse_button == -1)
			break;
		io.AddMouseButtonEvent(mouse_button, (event->type == SDL_MOUSEBUTTONDOWN));
		//bd->MouseButtonsDown = (event->type == SDL_MOUSEBUTTONDOWN) ? (bd->MouseButtonsDown | (1 << mouse_button)) : (bd->MouseButtonsDown & ~(1 << mouse_button));
		return true;
	}
	case SDL_TEXTINPUT:
	{
		io.AddInputCharactersUTF8(event->text.text);
		return true;
	}
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	{
		ImGuiIO& io = ImGui::GetIO();
		bool ctrl = event->key.keysym.mod & KMOD_CTRL;
		bool shift = event->key.keysym.mod & KMOD_SHIFT;
		bool alt = event->key.keysym.mod & KMOD_ALT;
		bool super = event->key.keysym.mod & KMOD_GUI;
		io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);

		io.AddKeyEvent(ImGuiMod_Shift, shift);
		io.AddKeyEvent(ImGuiMod_Alt, alt);
		io.AddKeyEvent(ImGuiMod_Super, super);

		ImGuiKey key = KeycodeToImGuiKey(event->key.keysym.sym);
		io.AddKeyEvent(key, (event->type == SDL_KEYDOWN));
		io.SetKeyEventNativeData(key, event->key.keysym.sym, event->key.keysym.scancode, event->key.keysym.scancode); // To support legacy indexing (<1.87 user code). Legacy backend uses SDLK_*** as indices to IsKeyXXX() functions.
		return true;
	}
	case SDL_WINDOWEVENT:
	{
		// - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the final ENTER tends to be right.
		// - However we won't get a correct LEAVE event for a captured window.
		// - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too late,
		//   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This is why
		//   we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
		Uint8 window_event = event->window.event;
		/*if (window_event == SDL_WINDOWEVENT_ENTER)
			bd->PendingMouseLeaveFrame = 0;
		if (window_event == SDL_WINDOWEVENT_LEAVE)
			bd->PendingMouseLeaveFrame = ImGui::GetFrameCount() + 1;*/
		if (window_event == SDL_WINDOWEVENT_FOCUS_GAINED)
			io.AddFocusEvent(true);
		else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			io.AddFocusEvent(false);
		return true;
	}


	}
	return false;
}

//bool UI::BeginMainStatusBar() {
//	ImGuiContext& g = *GImGui;
//	ImGuiViewportP* viewport = g.Viewports[0];
//	ImGuiWindow* menu_bar_window = ImGui::FindWindowByName("##MainStatusBar");
//	// For the main menu bar, which cannot be moved, we honor g.Style.DisplaySafeAreaPadding to ensure text can be visible on a TV set.
//	g.NextWindowData.MenuBarOffsetMinVal = ImVec2(g.Style.DisplaySafeAreaPadding.x, ImMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
//
//	// Get our rectangle at the top of the work area
//	//__debugbreak();
//	if (menu_bar_window == NULL || menu_bar_window->BeginCount == 0)
//	{
//		// Set window position
//		// We don't attempt to calculate our height ahead, as it depends on the per-viewport font size. However menu-bar will affect the minimum window size so we'll get the right height.
//		ImVec2 menu_bar_pos = viewport->Pos + viewport->BuildWorkOffsetMin;
//		ImVec2 menu_bar_size = ImVec2(viewport->Size.x - viewport->BuildWorkOffsetMin.x + viewport->BuildWorkOffsetMax.x, 1.0f);
//		ImGui::SetNextWindowPos(menu_bar_pos);
//		ImGui::SetNextWindowSize(menu_bar_size);
//	}
//
//	// Create window
//	ImGui::SetNextWindowViewport(viewport->ID); // Enforce viewport so we don't create our own viewport when ImGuiConfigFlags_ViewportsNoMerge is set.
//	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
//	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));    // Lift normal size constraint, however the presence of a menu-bar will give us the minimum height we want.
//	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
//	bool is_open = ImGui::Begin("##MainStatusBar", NULL, window_flags) && ImGui::BeginMenuBar();
//	ImGui::PopStyleVar(2);
//
//	// Report our size into work area (for next frame) using actual window size
//	menu_bar_window = ImGui::GetCurrentWindow();
//	if (menu_bar_window->BeginCount == 1)
//		viewport->BuildWorkOffsetMin.y += menu_bar_window->Size.y;
//
//	g.NextWindowData.MenuBarOffsetMinVal = ImVec2(0.0f, 0.0f);
//	if (!is_open)
//	{
//		ImGui::End();
//		return false;
//	}
//	return true; //-V1020
//}
//
//void UI::EndMainStatusBar() {
//	ImGui::EndMenuBar();
//
//	// When the user has left the menu layer (typically: closed menus through activation of an item), we restore focus to the previous window
//	// FIXME: With this strategy we won't be able to restore a NULL focus.
//	ImGuiContext& g = *GImGui;
//	if (g.CurrentWindow == g.NavWindow && g.NavLayer == ImGuiNavLayer_Main && !g.NavAnyRequest)
//		ImGui::FocusTopMostWindowUnderOne(g.NavWindow, NULL);
//
//	ImGui::End();
//}

bool UI::BeginMainStatusBar() {
	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();
	return ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, window_flags);
}

bool UI::BeginSecondaryMenuBar() {
	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();
	return ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Down, height, window_flags);
}

bool UI::ToggleButton(const char* str_id, bool* v) {
	ImVec2 p = ImGui::GetCursorScreenPos();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	float height = ImGui::GetFrameHeight();
	float width = height * 1.55f;
	float radius = height * 0.50f;

	ImGui::InvisibleButton(str_id, ImVec2(width, height));
	if (ImGui::IsItemClicked())
		*v = !*v;

	float t = *v ? 1.0f : 0.0f;

	ImGuiContext& g = *GImGui;
	float ANIM_SPEED = 0.08f;
	if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
	{
		float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
		t = *v ? (t_anim) : (1.0f - t_anim);
	}

	ImU32 col_bg;
	if (ImGui::IsItemHovered())
		col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
	else
		col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

	draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
	draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
	return *v;
}


