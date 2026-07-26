#include "DebugRenderer.h"
#include "Core/Application.h"
#include "imgui.h"

Mupfel::DebugRenderer::DebugRenderer(uint32_t frames_in_flight) : SubRenderer(frames_in_flight) {}

bool Mupfel::DebugRenderer::Init(const Ping::Device& device, Ping::Format swapChainFormat)
{
	logger = Logger::Create("Debug Renderer");
	gui = device.CreateGui(Application::Get().window.GetGLFWHandle(), swapChainFormat, framesInFlight);

	if (!gui)
	{
		logger->error("Unable to create GUI!");
		return false;
	}

	return true;
}

void Mupfel::DebugRenderer::PreUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer) {
	gui.value().NewFrame();
}

void Mupfel::DebugRenderer::PostUser(const Ping::Device& device, Ping::CommandBuffer& current_command_buffer)
{
	ImGui::ShowMetricsWindow();
	current_command_buffer.DrawGui(device, gui.value(), frameIndex);
	IncrementFrameIndex();
}
