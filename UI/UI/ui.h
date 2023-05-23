#pragma once

#include <VulkanLib.h>
#include <imgui.h>
#include "imgui-knobs.h"


class UI {
	VulkContext context;
	struct IGPushConstants {
		glm::vec2 scale;
		glm::vec2 translate;
	}pushConst;
	u8* vertexPtr{ nullptr };
	u8* indexPtr{ nullptr };
	Vulkan::Buffer	vertexBuffer;
	Vulkan::Buffer	indexBuffer;
	std::unique_ptr<VulkanVIBuffer> vertexBufferPtr;
	std::unique_ptr<VulkanVIBuffer> indexBufferPtr;
	VkDeviceSize vertexBufferSize{ 0 };
	VkDeviceSize indexBufferSize{ 0 };

	Vulkan::Texture fontTexture;
	std::unique_ptr<VulkanTexture> fontTexturePtr;
	VkDescriptorSetLayout descriptorLayout;
	std::unique_ptr<VulkanDescriptor> descriptorPtr;
	VkDescriptorSet	descriptor;
	std::unique_ptr<VulkanPipelineLayout> pipelineLayoutPtr;
	std::unique_ptr<VulkanPipeline> pipelinePtr;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	bool demoWindow{ false };
	u64		time{ 0 };
	ImGuiKey KeycodeToImGuiKey(i32 keyCode);
public:
	UI();
	~UI();

	void Init(VulkContext& context, VkRenderPass renderPass,const char*shaderPath=nullptr);
	void Resize(u32 width, u32 height);
	void Update();
	void NewFrame() { ImGui::NewFrame(); }
	void BeginWindow(const char* title, ImVec2 pos = ImVec2(0, 0), ImVec2 size = ImVec2(200, 200)) {
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		ImGui::Begin(title);

	}
	void Text(const char* text) { ImGui::Text(text); }
	bool Button(const char* text) { return ImGui::Button(text); }
	bool DragInt(const char* label, int* v, float v_speed = 1.f, int v_min = 0, int v_max = 100, const char* format = "%d", ImGuiSliderFlags flags = 0) {
		return ImGui::DragInt(label, v, v_speed, v_min, v_max, format, flags);
	}
	bool DragFloat(const char* text, float* v, float v_speed = 1.f, float v_min = 0.f, float v_max = 1.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
		return ImGui::DragFloat(text, v, v_speed, v_min, v_max, format, flags);
	}
	bool Radio(const char* text, bool val) {
		return ImGui::RadioButton(text, val);
	}
	void SameLine(float offset = 0.f, float spacing = -1.f) {
		ImGui::SameLine(offset, spacing);
	}
	void EndWindow() { ImGui::End(); }
	void DrawDemoWindow() { ImGui::ShowDemoWindow(&demoWindow); }
	void Render(VkCommandBuffer cmd, u32 width, u32 height);
	bool ProcessSDLEvent(const SDL_Event* event);
	bool Combo(const char* label,std::vector<const char*>& items, int &selItem,ImGuiComboFlags flags=0) {
		//ImGui::Text(label);
		//ImGui::SameLine();
		if (ImGui::BeginCombo(label, selItem >= 0 && selItem < (int)items.size() ? items[selItem] : nullptr, flags)) {
			for (int i = 0; i < (int)items.size(); ++i) {
				bool isSelected = (i == selItem);
				if (ImGui::Selectable(items[i], isSelected))
					selItem = i;
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
			return true;
		}
		return false;
	}
	bool Knob(const char* label, float* value, float min, float max, float speed=0, const char* format=nullptr, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0.f, ImGuiKnobFlags flags = ImGuiKnobFlags_ValueTooltip, int steps = 10) {
		return ImGuiKnobs::Knob(label, value, min, max, speed, format, variant, size, flags, steps);
	}
	bool KnobInt(const char* label, int* p_value, int v_min, int v_max, float speed = 0, const char* format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10) {
		return ImGuiKnobs::KnobInt(label, p_value, v_min, v_max, speed, format, variant, size, flags, steps);
	}
	bool BeginMainStatusBar();//from https://github.com/ocornut/imgui/issues/3518
	bool BeginSecondaryMenuBar();
	bool ToggleButton(const char* str, bool* v);
};