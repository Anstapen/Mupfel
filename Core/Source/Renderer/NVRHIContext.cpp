#include "NVRHIContext.h"
#include <iostream>

#include <vulkan/vulkan.hpp>

using namespace Mupfel;

Mupfel::NVRHIContext::~NVRHIContext() { Shutdown(); }

bool NVRHIContext::Init(GLFWwindow* window, uint32_t width, uint32_t height, uint32_t inFramesInFlight)
{
	if (!CreateInstanceAndSurface(window))
	{
		Shutdown();
		return false;
	}

	if (!PickPhysicalDevice())
	{
		Shutdown();
		return false;
	}

	if (!CreateLogicalDevice())
	{
		Shutdown();
		return false;
	}

	if (!CreateSwapchain(width, height, inFramesInFlight))
	{
		Shutdown();
		return false;
	}

	return true;
}

void Mupfel::NVRHIContext::Shutdown()
{
	DestroySemaphores();
	vkb::destroy_swapchain(this->swapchain);
	vkb::destroy_surface(this->instance, this->surface);
	vkb::destroy_device(this->device);
	vkb::destroy_instance(this->instance);

	this->swapchain = vkb::Swapchain();
	this->surface = VK_NULL_HANDLE;
	this->device = vkb::Device();
	this->phys_device = vkb::PhysicalDevice();
	this->instance = vkb::Instance();
}

bool Mupfel::NVRHIContext::CreateSwapchain(uint32_t width, uint32_t height, uint32_t inFramesInFlight)
{
	if (this->device.device == VK_NULL_HANDLE)
	{
		return false;
	}

	this->framesInFlight = inFramesInFlight;

	/* Build a new swapchain. */
	vkb::SwapchainBuilder swapchain_builder{this->device};
	swapchain_builder.set_desired_min_image_count(vkb::SwapchainBuilder::DOUBLE_BUFFERING);
	swapchain_builder.set_desired_format({VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});
	swapchain_builder.set_desired_extent(width, height);
	swapchain_builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	swapchain_builder.add_image_usage_flags(VK_IMAGE_USAGE_SAMPLED_BIT);

	/* If this is not the first time creating the swapchain, recycle the old one. */
	if (this->swapchain.swapchain != nullptr)
	{
		swapchain_builder.set_old_swapchain(this->swapchain);
	}

	auto swap_ret = swapchain_builder.build();

	/*
	 * We now destroy the old swapchain unconditionally.
	 * Triggering a swapchain recreation means the old swapchain is unusable.
	 */

	if (this->swapchain.swapchain != nullptr)
	{
		DestroySemaphores();
		vkb::destroy_swapchain(this->swapchain);
		this->swapchain = vkb::Swapchain();
	}

	if (!swap_ret)
	{
		std::cerr << "Failed to create Vulkan swapchain. Error: " << swap_ret.error().message() << "\n";
		return false;
	}

	/* New swapchain was created successfully, build new synchronization data. */

	if (!CreateSemaphores(swap_ret.value().image_count))
	{
		vkb::destroy_swapchain(swap_ret.value());
		return false;
	}

	this->swapchain = swap_ret.value();

	return true;
}

bool Mupfel::NVRHIContext::CreateSemaphores(uint32_t image_count)
{
	if (this->device.device == VK_NULL_HANDLE)
	{
		return false;
	}

	DestroySemaphores();

	bool semaphoreCreationFailed = false;

	/* Use a lambda for now to not duplicate code as much. */
	auto create_sems = [this, &semaphoreCreationFailed](std::vector<VkSemaphore>& sems, uint32_t count) -> bool
	{
		for (uint32_t i = 0; i < count; i++)
		{
			VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
			VkSemaphore			  sem = VK_NULL_HANDLE;

			if (vkCreateSemaphore(this->device.device, &info, nullptr, &sem) != VK_SUCCESS)
			{
				std::cerr << "Failed to create Vulkan Semaphore!\n";
				return false;
			}

			sems.push_back(sem);
		}

		return true;
	};

	if (!create_sems(this->presentSemaphores, image_count))
	{
		semaphoreCreationFailed = true;
	}

	if (!create_sems(this->acquireSemaphores, this->framesInFlight))
	{
		semaphoreCreationFailed = true;
	}

	this->acquireSemaphoreIndex = 0;

	/* If one of the semaphores could not be created, do not leave the vectors in an undefined state.
	 * We either need all semaphores to be present or none at all.
	 */
	if (semaphoreCreationFailed)
	{
		DestroySemaphores();
		return false;
	}

	return true;
}

void Mupfel::NVRHIContext::DestroySemaphores()
{
	if (this->device.device == VK_NULL_HANDLE)
	{
		return;
	}

	for (VkSemaphore sem : this->presentSemaphores)
	{
		vkDestroySemaphore(this->device.device, sem, nullptr);
	}

	this->presentSemaphores.clear();

	for (VkSemaphore sem : this->acquireSemaphores)
	{
		vkDestroySemaphore(this->device.device, sem, nullptr);
	}

	this->acquireSemaphores.clear();

	this->acquireSemaphoreIndex = 0;
}

bool Mupfel::NVRHIContext::CreateInstanceAndSurface(GLFWwindow* window)
{
	vkb::InstanceBuilder instance_builder;

	/* Let vk-bootstrap gather information about related layers and extensions. */
	auto system_info_ret = vkb::SystemInfo::get_system_info();
	if (!system_info_ret)
	{
		std::cerr << system_info_ret.error().message().c_str() << "\n";
		return false;
	}
	auto& system_info = system_info_ret.value();

	/* Use validation layers only in debug builds. */
	if (useValidation)
	{
		if (system_info.validation_layers_available)
		{
			instance_builder.enable_validation_layers().use_default_debug_messenger().set_debug_messenger_type(
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
		}
	}

	/* We require Vulkan 1.3. */
	instance_builder.require_api_version(1, 3, 0);

	auto inst_ret = instance_builder.build();
	if (!inst_ret)
	{
		std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
		return false;
	}
	this->instance = inst_ret.value();

	VULKAN_HPP_DEFAULT_DISPATCHER.init(this->instance.fp_vkGetInstanceProcAddr);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Instance(this->instance.instance));

	VkResult err = glfwCreateWindowSurface(this->instance, window, NULL, &this->surface);
	if (err != VK_SUCCESS)
	{
		std::cerr << "Failed to create Window Surface.\n";
		return false;
	}

	return true;
}

bool Mupfel::NVRHIContext::PickPhysicalDevice()
{
	vkb::PhysicalDeviceSelector phys_device_selector(this->instance);
	phys_device_selector.set_surface(this->surface);

	VkPhysicalDeviceVulkan11Features required_11features{};
	required_11features.shaderDrawParameters = true;

	VkPhysicalDeviceVulkan12Features required_12features{};
	required_12features.timelineSemaphore = true;
	required_12features.descriptorBindingPartiallyBound = true;
	required_12features.shaderSampledImageArrayNonUniformIndexing = true;

	VkPhysicalDeviceVulkan13Features required_13features{};
	required_13features.synchronization2 = true;
	required_13features.dynamicRendering = true;

	phys_device_selector.set_required_features_11(required_11features);
	phys_device_selector.set_required_features_12(required_12features);
	phys_device_selector.set_required_features_13(required_13features);

	auto physical_device_selector_return = phys_device_selector.select();
	if (!physical_device_selector_return)
	{

		if (physical_device_selector_return.error() == vkb::PhysicalDeviceError::no_suitable_device)
		{
			const auto& detailed_reasons = physical_device_selector_return.detailed_failure_reasons();
			if (!detailed_reasons.empty())
			{
				std::cerr << "GPU Selection failure reasons:\n";
				for (const std::string& reason : detailed_reasons)
				{
					std::cerr << reason << "\n";
				}
			}
		}
		return false;
	}

	this->phys_device = physical_device_selector_return.value();

	return true;
}

bool Mupfel::NVRHIContext::CreateLogicalDevice()
{
	vkb::DeviceBuilder device_builder{this->phys_device};
	auto			   dev_ret = device_builder.build();
	if (!dev_ret)
	{
		return false;
	}
	this->device = dev_ret.value();

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device(this->device.device));

	auto queue_and_index = this->device.get_queue_and_index(vkb::QueueType::graphics);

	if (!queue_and_index)
	{
		return false;
	}

	this->graphics_q = queue_and_index.value().first;
	this->graphics_q_index = queue_and_index.value().second;

	queue_and_index = this->device.get_queue_and_index(vkb::QueueType::transfer);

	if (!queue_and_index)
	{
		return false;
	}

	this->transfer_q = queue_and_index.value().first;
	this->transfer_q_index = queue_and_index.value().second;

	queue_and_index = this->device.get_queue_and_index(vkb::QueueType::compute);

	if (!queue_and_index)
	{
		return false;
	}

	this->compute_q = queue_and_index.value().first;
	this->compute_q_index = queue_and_index.value().second;

	queue_and_index = this->device.get_queue_and_index(vkb::QueueType::present);

	if (!queue_and_index)
	{
		return false;
	}

	this->present_q = queue_and_index.value().first;
	this->present_q_index = queue_and_index.value().second;

	return true;
}
