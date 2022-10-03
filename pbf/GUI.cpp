#include <pbf/GUI.h>
#include <pbf/Context.h>
#include <pbf/Renderer.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/RenderPass.h>
#include <pbf/Selection.h>
#include <pbf/Scene.h>

namespace pbf {

GUI::GUI(pbf::InitContext &initContext): _context(initContext.context), _selection(initContext)
{
	// TODO: error handling
	_imguiContext = ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(_context.window().window(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _context.instance();
	init_info.PhysicalDevice = _context.physicalDevice();
	init_info.Device = _context.device();
	init_info.QueueFamily = _context.graphicsQueueFamily();
	init_info.Queue = _context.graphicsQueue();
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = _context.descriptorPool();
	init_info.Subpass = 0;
	init_info.MinImageCount = _context.renderer().framePrerenderCount();
	init_info.ImageCount = _context.renderer().framePrerenderCount();
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = [](VkResult result) {
		// TODO
	};
	ImGui_ImplVulkan_Init(&init_info, *_context.renderer().renderPass());

	ImGui_ImplVulkan_CreateFontsTexture(*initContext.initCommandBuffer);
}

GUI::~GUI()
{
	ImGui::SetCurrentContext(_imguiContext);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(_imguiContext);
}

void GUI::postInitCleanup()
{
	std::lock_guard guard(_imguiMutex);
	ImGui::SetCurrentContext(_imguiContext);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void GUI::render(vk::CommandBuffer buf)
{
	std::lock_guard guard(_imguiMutex);

	ImGui::SetCurrentContext(_imguiContext);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (ImGui::IsKeyDown(ImGuiKey_H) && ImGui::IsMouseClicked(ImGuiMouseButton_Left, false))
	{
		auto [xpos, ypos] = _context.window().getCursorPos();
		if (auto index = _selection(_context.scene().particleData(), xpos, ypos))
		{
			spdlog::get("console")->debug("Clicked on particle {}", *index);

			auto cmdBuffer = std::move(_context.device().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{
				.commandPool = _context.commandPool(true),
				.level = vk::CommandBufferLevel::ePrimary,
				.commandBufferCount = 1U
			}).front());

			cmdBuffer->begin(vk::CommandBufferBeginInfo{});
			for (size_t i = 0; i <= _context.renderer().framePrerenderCount(); ++i)
			{
				auto segment = _context.scene().particleData().segment(i);
				cmdBuffer->fillBuffer(
					segment.buffer,
					segment.offset + sizeof(ParticleData) * *index + offsetof(ParticleData, aux),
					sizeof(ParticleData::aux),
					-1u
				);
			}
			cmdBuffer->end();

			_context.graphicsQueue().waitIdle();
			_context.graphicsQueue().submit({
												vk::SubmitInfo{
													.waitSemaphoreCount = 0,
													.pWaitSemaphores = nullptr,
													.pWaitDstStageMask = {},
													.commandBufferCount = 1,
													.pCommandBuffers = &*cmdBuffer,
													.signalSemaphoreCount = 0,
													.pSignalSemaphores = nullptr
												}
											});
			_context.graphicsQueue().waitIdle();
		}
	}

	ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
	ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
	auto [width, height] = _context.window().framebufferSize();
	dragDelta.x /= float(width);
	dragDelta.y /= float(height);

	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
	{
		_context.camera().Zoom(dragDelta.x + dragDelta.y);
	}
	else if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
	{
		_context.camera().MoveX (dragDelta.x);
		_context.camera().MoveY (dragDelta.y);
	}
	else
	{
		_context.camera().Rotate (dragDelta.y, -dragDelta.x);
	}


	{
		ImGui::Begin("PBF", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (ImGui::Button(_runSPH ? "Stop" : "Run"))
			_runSPH = !_runSPH;
		ImGui::SameLine();
		ImGui::Text(_runSPH ? "SPH is running" : "SPH is not running");


		static auto lastTime = std::chrono::steady_clock::now();
		static size_t frameCount = 0;
		static size_t FPS = 0;
		frameCount++;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastTime).count() >= 1000) {
			lastTime = std::chrono::steady_clock::now();
			FPS = frameCount;
			frameCount = 0;
		}
		ImGui::Text("FPS %d", FPS);

		ImGui::End();
	}

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();

	ImGui_ImplVulkan_RenderDrawData(drawData, buf);
}

}