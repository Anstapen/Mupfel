#include "IMRenderer.h"
#include "Core/Application.h"
#include "Quad.h"
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct TextureInstance
{
	float	 pos_x = 0.0f;
	float	 pos_y = 0.0f;
	float	 width = 1.0f;
	float	 height = 1.0f;
	float	 rotation = 0.0f;
	uint32_t index = 1;
	float	 uvScale = 1.0f;
	float	 _pad0;
};

static const uint32_t default_entity_capacity = 1000;

bool Mupfel::IMRenderer::Init(
	nvrhi::DeviceHandle			  device,
	ImageManager&				  img_manager,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)device;
	(void)frameBufferInfo;
	(void)framesInFlight;
	(void)img_manager;
	logger = Logger::Create("Immediate Mode Renderer");
	return true;
}

void Mupfel::IMRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;
}

void Mupfel::IMRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	ImageManager&			 img_manager,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)img_manager;
	(void)current_command_list;
	(void)context;
}

uint32_t Mupfel::IMRenderer::Button(float x, float y, float width, float height, const std::string& image_path)
{
	if (!UploadImage(image_path))
	{
		/* Something went wrong uploading the image. */
		return 0;
	}
	auto it = images.find(image_path);

	ImageHandle image_h = it->second[0];

	uint32_t return_value = 0;

	double cursor_x = Application::GetCurrentInputManager().GetCurrentCursorX();
	double cursor_y = Application::GetCurrentInputManager().GetCurrentCursorY();

	/* Check collision with the button. */
	bool hovering = (cursor_x < x + width && cursor_x > x && cursor_y < y + height && cursor_y > y);

	if (hovering)
	{
		/* The button is released. TODO: this search is linear currently! */
		if (Application::GetCurrentInputManager().CheckUserInput(UserInput::LEFT_MOUSE_CLICK, KeyAction::RELEASED))
		{
			image_h = it->second[2];
			return_value = 3;
		}
		else if (Application::GetCurrentInputManager().CheckUserInput(UserInput::LEFT_MOUSE_CLICK, KeyAction::PRESSED))
		{
			image_h = it->second[2];
			return_value = 2;
		}
		else
		{
			image_h = it->second[1];
			return_value = 1;
		}
	}

	PushObject(x, y, width, height, 0.0f, image_h, 1.0f);

	return return_value;
}

bool Mupfel::IMRenderer::UploadImage(const std::string& image_path)
{
	/* Check if the given texture was already used before. */
	auto it = images.find(image_path);

	if (it == images.end())
	{
		/* Load it from disk. The image needs to be a spritesheet with one row, containing 3 images, in the following
		 * order:
		 * 1. The unhovered button.
		 * 2. The hovered button.
		 * 3. The pressed button.
		 */
		auto image_handles = Application::LoadSpriteSheetImages(image_path, {.rows = 1, .columns = 3});

		if (!image_handles)
		{
			return false;
		}

		images[image_path] = image_handles.value();
	}

	/* We should only get here if the image upload succeeded... */
	assert(images.find(image_path) != images.end());

	return true;
}

void Mupfel::IMRenderer::PushObject(
	float	 x,
	float	 y,
	float	 width,
	float	 height,
	float	 rotation,
	uint32_t index,
	float	 uv_scale)
{
	const float screen_w = static_cast<float>(Application::GetCurrentRenderWidth());
	const float screen_h = static_cast<float>(Application::GetCurrentRenderHeight());

	if (screen_w <= 0.0f || screen_h <= 0.0f)
	{
		return;
	}

	/* The quad spans [-0.5, 0.5] around its centre, so convert the top-left pixel rect
	 * into an NDC centre plus an NDC extent. */
	TextureInstance t{};
	t.pos_x = ((x + width * 0.5f) / screen_w) * 2.0f - 1.0f;
	t.pos_y = 1.0f - ((y + height * 0.5f) / screen_h) * 2.0f;
	t.width = (width / screen_w) * 2.0f;
	t.height = (height / screen_h) * 2.0f;
	t.rotation = rotation;
	t.index = index;
	t.uvScale = uv_scale;

	/* TODO: push the object into the gpu buffer */
}
