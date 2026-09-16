#include "DebugRenderer.h"
#include "Core/Application.h"
#include "imgui_impl_glfw.h"

#include "Shaders/imgui_fragment.h"
#include "Shaders/imgui_vertex.h"

bool Mupfel::DebugRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)img_manager;
	(void)framesInFlight;
	logger = Logger::Create("Debug Renderer");

	ImGuiContext* imgui_context = ImGui::CreateContext();
	ImGui::SetCurrentContext(imgui_context);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(Application::Get().window.GetGLFWHandle(), true);

	nvrhi::ShaderHandle vertexShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Vertex).setEntryName("vertMain"), imguiVertex,
		imguiVertex_sizeInBytes);
	/*
	 * TODO: check returned shader handle, once the unrecoverable error handling is in place.
	 * A missing vertex shader in the debug renderer in unrecoverable!
	 */

	nvrhi::VertexAttributeDesc attributes[] = {
		nvrhi::VertexAttributeDesc()
			.setName("POSITION")
			.setFormat(nvrhi::Format::RG32_FLOAT)
			.setOffset(offsetof(ImDrawVert, pos))
			.setElementStride(sizeof(ImDrawVert)),
		nvrhi::VertexAttributeDesc()
			.setName("TEXCOORD")
			.setFormat(nvrhi::Format::RG32_FLOAT)
			.setOffset(offsetof(ImDrawVert, uv))
			.setElementStride(sizeof(ImDrawVert)),
		nvrhi::VertexAttributeDesc()
			.setName("COLOR")
			.setFormat(nvrhi::Format::RGBA8_UNORM)
			.setOffset(offsetof(ImDrawVert, col))
			.setElementStride(sizeof(ImDrawVert))};

	nvrhi::InputLayoutHandle inputLayout =
		device->createInputLayout(attributes, uint32_t(std::size(attributes)), vertexShader);

	nvrhi::ShaderHandle fragmentShader = device->createShader(
		nvrhi::ShaderDesc().setShaderType(nvrhi::ShaderType::Pixel).setEntryName("fragMain"), imguiFragment,
		imguiFragment_sizeInBytes);

	/*
	 * TODO: check returned shader handle, once the unrecoverable error handling is in place.
	 * A missing fragment shader in the debug renderer in unrecoverable!
	 */

	auto layoutDesc = nvrhi::BindingLayoutDesc()
						  .setVisibility(nvrhi::ShaderType::All)
						  .setBindingOffsets(
							  nvrhi::VulkanBindingOffsets()
								  .setShaderResourceOffset(0)
								  .setSamplerOffset(0)
								  .setConstantBufferOffset(0)
								  .setUnorderedAccessViewOffset(0))
						  .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0).setSize(1))
						  .addItem(nvrhi::BindingLayoutItem::Sampler(1))
						  .addItem(nvrhi::BindingLayoutItem::PushConstants(2, sizeof(float) * 4));

	bindingLayout = device->createBindingLayout(layoutDesc);

	auto pipelineDesc =
		nvrhi::GraphicsPipelineDesc()
			.setInputLayout(inputLayout)
			.addBindingLayout(bindingLayout)
			.setVertexShader(vertexShader)
			.setPixelShader(fragmentShader)
			.setRenderState(
				nvrhi::RenderState()
					.setDepthStencilState(nvrhi::DepthStencilState().disableDepthTest().disableDepthWrite())
					.setRasterState(
						nvrhi::RasterState()
							.setCullMode(nvrhi::RasterCullMode::None)
							.setFrontCounterClockwise(true)
							.enableQuadFill())
					.setBlendState(
						nvrhi::BlendState().setRenderTarget(
							0, nvrhi::BlendState::RenderTarget()
								   .enableBlend()
								   .setSrcBlend(nvrhi::BlendFactor::SrcAlpha)
								   .setDestBlend(nvrhi::BlendFactor::OneMinusSrcAlpha)
								   .setBlendOp(nvrhi::BlendOp::Add)
								   .setSrcBlendAlpha(nvrhi::BlendFactor::One)
								   .setDestBlendAlpha(nvrhi::BlendFactor::OneMinusSrcAlpha))));

	this->pipeline = device->createGraphicsPipeline(pipelineDesc, frameBufferInfo);

	if (!this->pipeline)
	{
		/* TODO: this is an unrecoverable error! */
		logger->critical("Unable to create graphics pipeline!");
		return false;
	}

	sampler = device->createSampler(
		nvrhi::SamplerDesc().setAllFilters(false).setAllAddressModes(nvrhi::SamplerAddressMode::ClampToEdge));

	/* TODO: The sampler can be nullptr, unrecoverable! */

	unsigned char* fontPixels;
	int			   fontWidth, fontHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontPixels, &fontWidth, &fontHeight);

	nvrhi::TextureDesc desc = nvrhi::TextureDesc()
								  .setDimension(nvrhi::TextureDimension::Texture2D)
								  .setWidth(fontWidth)
								  .setHeight(fontHeight)
								  .setArraySize(1)
								  .setFormat(nvrhi::Format::RGBA8_UNORM)
								  .enableAutomaticStateTracking(nvrhi::ResourceStates::ShaderResource)
								  .setDebugName("ImguiFontImage");

	nvrhi::CommandListHandle uploadList =
		device->createCommandList(nvrhi::CommandListParameters().setEnableImmediateExecution(false));
	if (!uploadList)
	{
		/* TODO: this is an unrecoverable error! */
		logger->critical("Unable to create upload command list!");
		return false;
	}

	fontImage = device->createTexture(desc);

	if (!fontImage)
	{
		/* TODO: this is an unrecoverable error! */
		logger->critical("Unable to create font atlas!");
		return false;
	}

	uploadList->open();
	uploadList->writeTexture(fontImage, 0, 0, fontPixels, static_cast<size_t>(fontWidth) * 4);
	uploadList->setPermanentTextureState(fontImage, nvrhi::ResourceStates::ShaderResource);
	uploadList->commitBarriers();
	uploadList->close();
	device->executeCommandList(uploadList);

	bindingSet = device->createBindingSet(
		nvrhi::BindingSetDesc()
			.addItem(nvrhi::BindingSetItem::Texture_SRV(0, fontImage))
			.addItem(nvrhi::BindingSetItem::Sampler(1, sampler))
			.addItem(nvrhi::BindingSetItem::PushConstants(2, sizeof(float) * 4)),
		bindingLayout);

	if (!bindingSet)
	{
		/* TODO: this is an unrecoverable error! */
		logger->critical("Unable to create the descriptor set!");
		return false;
	}

	io.Fonts->SetTexID(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(fontImage.Get())));

	return true;
}

void Mupfel::DebugRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Mupfel::DebugRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)img_manager;


	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	if (!draw_data || draw_data->CmdListsCount == 0 || draw_data->DisplaySize.x <= 0.0f ||
		draw_data->DisplaySize.y <= 0.0f)
	{
		return;
	}

	size_t vertexSize = static_cast<size_t>(draw_data->TotalVtxCount) * sizeof(ImDrawVert);
	size_t indexSize = static_cast<size_t>(draw_data->TotalIdxCount) * sizeof(ImDrawIdx);

	if (!CheckVertexBufferCapacity(device, vertexSize) || !CheckIndexBufferCapacity(device, indexSize))
	{
		logger->error("Unable to allocate new GPU buffers!");
		return;
	}

	uint64_t vertexOffset = 0;
	uint64_t indexOffset = 0;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const size_t	  vertexBytes = static_cast<size_t>(cmd_list->VtxBuffer.Size) * sizeof(ImDrawVert);
		const size_t	  indexBytes = static_cast<size_t>(cmd_list->IdxBuffer.Size) * sizeof(ImDrawIdx);

		if (vertexBytes > 0)
		{
			current_command_list->writeBuffer(vertexBuffer, cmd_list->VtxBuffer.Data, vertexBytes, vertexOffset);
		}

		if (indexBytes > 0)
		{
			current_command_list->writeBuffer(indexBuffer, cmd_list->IdxBuffer.Data, indexBytes, indexOffset);
		}

		vertexOffset += vertexBytes;
		indexOffset += indexBytes;
	}

	/* We are y up, ImGui is y down, so we need to convert. */
	const float pushConstants[4] = {
		2.0f / draw_data->DisplaySize.x, -2.0f / draw_data->DisplaySize.y,
		-1.0f - draw_data->DisplayPos.x * (2.0f / draw_data->DisplaySize.x),
		1.0f + draw_data->DisplayPos.y * (2.0f / draw_data->DisplaySize.y)};

	const int fb_width = static_cast<int>(context.width);
	const int fb_height = static_cast<int>(context.height);

	nvrhi::GraphicsState state =
		nvrhi::GraphicsState()
			.setPipeline(pipeline)
			.setFramebuffer(context.frameBuffer)
			.setViewport(
				nvrhi::ViewportState()
					.addViewport(nvrhi::Viewport(float(fb_width), float(fb_height)))
					.addScissorRect(nvrhi::Rect(fb_width, fb_height)))
			.addBindingSet(bindingSet)
			.addVertexBuffer(nvrhi::VertexBufferBinding().setBuffer(vertexBuffer).setSlot(0).setOffset(0))
			.setIndexBuffer(
				nvrhi::IndexBufferBinding()
					.setBuffer(indexBuffer)
					.setFormat(sizeof(ImDrawIdx) == 2 ? nvrhi::Format::R16_UINT : nvrhi::Format::R32_UINT)
					.setOffset(0));

	const ImVec2 clip_off = draw_data->DisplayPos;
	const ImVec2 clip_scale = draw_data->FramebufferScale;

	uint32_t global_vtx_offset = 0;
	uint32_t global_idx_offset = 0;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd& cmd = cmd_list->CmdBuffer[cmd_i];

			ImVec2 clip_min((cmd.ClipRect.x - clip_off.x) * clip_scale.x, (cmd.ClipRect.y - clip_off.y) * clip_scale.y);
			ImVec2 clip_max((cmd.ClipRect.z - clip_off.x) * clip_scale.x, (cmd.ClipRect.w - clip_off.y) * clip_scale.y);

			/* vkCmdSetScissor rejects negative offsets and extents past the framebuffer. */
			clip_min.x = std::max(clip_min.x, 0.0f);
			clip_min.y = std::max(clip_min.y, 0.0f);
			clip_max.x = std::min(clip_max.x, static_cast<float>(fb_width));
			clip_max.y = std::min(clip_max.y, static_cast<float>(fb_height));

			if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
			{
				continue;
			}

			state.viewport.scissorRects[0] = nvrhi::Rect(
				static_cast<int>(clip_min.x), static_cast<int>(clip_max.x), static_cast<int>(clip_min.y),
				static_cast<int>(clip_max.y));

			current_command_list->setGraphicsState(state);
			current_command_list->setPushConstants(pushConstants, sizeof(pushConstants));
			current_command_list->drawIndexed(
				nvrhi::DrawArguments()
					.setVertexCount(cmd.ElemCount)
					.setStartIndexLocation(cmd.IdxOffset + global_idx_offset)
					.setStartVertexLocation(cmd.VtxOffset + global_vtx_offset));
		}

		global_idx_offset += static_cast<uint32_t>(cmd_list->IdxBuffer.Size);
		global_vtx_offset += static_cast<uint32_t>(cmd_list->VtxBuffer.Size);
	}
}

bool Mupfel::DebugRenderer::CheckVertexBufferCapacity(nvrhi::DeviceHandle device, size_t required_size)
{
	if (vertexBufferSize >= required_size)
	{
		return true;
	}

	auto vertexBufferDesc = nvrhi::BufferDesc()
								.setByteSize(required_size)
								.setIsVertexBuffer(true)
								.enableAutomaticStateTracking(nvrhi::ResourceStates::VertexBuffer)
								.setDebugName("Vertex Buffer");

	nvrhi::BufferHandle new_buffer = device->createBuffer(vertexBufferDesc);

	if (!new_buffer)
	{
		return false;
	}

	vertexBuffer = new_buffer;
	vertexBufferSize = required_size;

	return true;
}

bool Mupfel::DebugRenderer::CheckIndexBufferCapacity(nvrhi::DeviceHandle device, size_t required_size)
{
	if (indexBufferSize >= required_size)
	{
		return true;
	}

	auto vertexBufferDesc = nvrhi::BufferDesc()
								.setByteSize(required_size)
								.setIsIndexBuffer(true)
								.enableAutomaticStateTracking(nvrhi::ResourceStates::IndexBuffer)
								.setDebugName("Index Buffer");

	nvrhi::BufferHandle new_buffer = device->createBuffer(vertexBufferDesc);

	if (!new_buffer)
	{
		return false;
	}

	indexBuffer = new_buffer;
	indexBufferSize = required_size;

	return true;
}
