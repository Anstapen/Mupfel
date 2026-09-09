#include "ECSRenderer.h"
#include "ECSRenderer.h"
#include "Core/Application.h"
#include "ImageManager.h"
#include "Quad.h"
#include "CameraMath.h"

#include "ECS/Components/Animation.h"
#include "ECS/Components/Light.h"
#include "ECS/Components/Texture.h"
#include "ECS/Components/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct UniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 cameraPos;
};

struct TextureInstance
{
	float	 pos_x = 0.0f;
	float	 pos_y = 0.0f;
	float	 pos_z = 0.0f;
	float	 _pad0;
	float	 scale_x = 1.0f;
	float	 scale_y = 1.0f;
	float	 rotation = 0.0f;
	uint32_t index = 1;
	float	 _pad1;
	uint32_t emitsLight = 0;
	uint32_t frame = 0;
	float	 _pad2;
};

struct LightInstance
{
	glm::vec3 colorRGB;
	float	  ambientStrength = 0.0f;
	glm::vec3 pos;
	float	  _pad1;
};

struct LightParams
{
	uint32_t numLights = 0;
	float	 _pad0 = 0.0f;
	float	 _pad1 = 0.0f;
	float	 _pad2 = 0.0f;
};


bool Mupfel::ECSRenderer::Init(
	nvrhi::DeviceHandle			  device,
	const nvrhi::FramebufferInfo& frameBufferInfo,
	uint32_t					  framesInFlight)
{
	(void)device;
	(void)frameBufferInfo;
	(void)framesInFlight;
	logger = Logger::Create("ECS Renderer");

	return true;
}

void Mupfel::ECSRenderer::PreUser(
	nvrhi::DeviceHandle		 device,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	/* Currently the ECS renderer does all the work after user input. */
	(void)device;
	(void)current_command_list;
	(void)context;
}

void Mupfel::ECSRenderer::PostUser(
	nvrhi::DeviceHandle		 device,
	nvrhi::CommandListHandle current_command_list,
	const FrameContext&		 context)
{
	(void)device;
	(void)current_command_list;
	(void)context;
}