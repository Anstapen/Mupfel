#pragma once
#include "ImageManager.h"
#include "Logger.h"
#include "SubRenderer.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Mupfel
{
class IMRenderer : public SubRenderer
{
public:
	bool Init(
		nvrhi::DeviceHandle			  device,
		ImageManager&				  img_manager,
		const nvrhi::FramebufferInfo& frameBufferInfo,
		uint32_t					  framesInFlight) final;
	void PreUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context)
		final;
	void PostUser(
		nvrhi::DeviceHandle		 device,
		ImageManager&			 img_manager,
		nvrhi::CommandListHandle current_command_list,
		const FrameContext&		 context) final;

	/**
	 * Display a button with the given texture.
	 *
	 * \param x x-offset in screen space.
	 * \param y y-offset in screen space.
	 * \param width The width of the button. This is independent of the used texture.
	 * \param height The height of the button. This is independent of the used texture.
	 * \param image_path Path to an image containing the button texture.
	 * \return 0 if the cursor is not overlapping the button, 1 if the cursor is hovering over the button, 2 if the
	 * button is pressed.
	 */
	uint32_t Button(float x, float y, float width, float height, const std::string& image_path);

private:
	bool UploadImage(const std::string& image_path);
	void PushObject(float x, float y, float width, float height, float rotation, uint32_t index, float uv_scale);

private:
	Logger::SafeLoggerPtr									  logger;
	std::unordered_map<std::string, std::vector<ImageHandle>> images;
	uint32_t												  currentImageCount = 0;
};
} // namespace Mupfel
