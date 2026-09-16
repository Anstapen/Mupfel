#pragma once
#include "nvrhi/nvrhi.h"
#include <cstdint>
#include <vector>

namespace Mupfel
{

/** Possible status return codes for functions of the DualBuffer class. */
enum class DualBufferStatus
{
	UPLOAD_SUCCESS,
	NO_UPLOAD_NEEDED,
	NO_RESIZE_NEEDED,
	BUFFER_RESIZED,
	BUFFER_ALLOCATION_FAILED
};

/**
 * This class combines a std::vector and NVRHI buffer to simplify resource handling of data that
 * is present in both CPU and GPU memory. It is mostly used by the different sub renderers. */
template <typename T> class DualBuffer
{
	/* The size of the object needs to be a multiple of 4 bytes. */
	static_assert(sizeof(T) % 4 == 0);

public:
	/**
	 * Default constructor.
	 *
	 * This constructor does not allocate a GPU buffer until FlushToGPU() is called.
	 *
	 */
	DualBuffer();

	/**
	 * Push an object into the CPU buffer.
	 *
	 * \param object
	 */
	void PushBack(const T& object) { cpuMem.push_back(object); }

	/**
	 * Clear the CPU buffer.
	 *
	 */
	void Clear() { cpuMem.clear(); }

	/**
	 * Wether or not the CPU buffer is empty.
	 */
	bool Empty() const { return cpuMem.empty(); }

	/**
	 * Return the size of the CPU buffer.
	 *
	 * \return The size of the CPU buffer.
	 */
	size_t Size() const { return cpuMem.size(); }

	/**
	 * Return the underlying GPU buffer handle.
	 * 
	 * \return The GPU buffer handle.
	 */
	nvrhi::BufferHandle GetGPUBufferHandle() const { return handle; }

	/**
	 * Copies the contents of the CPU buffer into the GPU buffer.
	 *
	 * \note If the current GPU buffer is too small to hold all elements, this function attempts
	 * to resize it.
	 *
	 * \note If the CPU buffer is empty, this function does nothing.
	 *
	 * \param device The device to be used for a GPU buffer resize, in case the current GPU buffer is too small.
	 * \param current_command_list The command list to be used to upload the CPU data into the GPU buffer.
	 * \return A status code.
	 * \retval NO_UPLOAD_NEEDED The CPU buffer is empty, nothing to do.
	 * \retval BUFFER_ALLOCATION_FAILED The function attempted to resize the GPU buffer, but the allocation of the new
	 * buffer failed.
	 * \retval BUFFER_RESIZED The upload was successful, but the GPU buffer was resized.
	 * \retval UPLOAD_SUCCESS The upload was successful, no buffer resizing took place.
	 */
	DualBufferStatus FlushToGPU(nvrhi::DeviceHandle device, nvrhi::CommandListHandle current_command_list);

public:
	DualBuffer(const DualBuffer& other) = delete;
	DualBuffer& operator=(const DualBuffer& other) = delete;

private:
	/**
	 * Attempts to acquire a new GPU buffer that can hold \a new_size elements of T.
	 *
	 * \param device The device to be used for buffer allocation.
	 * \param new_size The number of elements of T the new buffer should hold.
	 * \return True, if the buffer was allocated successfully, false otherwise.
	 */
	bool increaseGPUBufferSize(nvrhi::DeviceHandle device, size_t new_size);

	/**
	 * This function checks the current GPU buffer capacity and allocates a new one
	 * if \a required_capacity is larger.
	 *
	 * \param device The device to be used for buffer allocation.
	 * \param required_capacity The new required capacity for the GPU buffer.
	 * \return A Status code.
	 * \retval NO_RESIZE_NEEDED The current GPU buffer capacity is large enough.
	 * \retval BUFFER_ALLOCATION_FAILED The GPU buffer needed a resize, but the given device was unable to allocate a
	 * new buffer.
	 * \retval BUFFER_RESIZED The GPU buffer was successfully resized to at least \a required_capacity.
	 */
	DualBufferStatus EnsureGPUCapacity(nvrhi::DeviceHandle device, size_t required_capacity);

private:
	/** The NVRHI handle to the GPU buffer. */
	nvrhi::BufferHandle handle;

	/** The current GPU buffer capacity. */
	size_t				gpuBufferCapacity;

	/** The CPU buffer. */
	std::vector<T>		cpuMem;
};

template <typename T> inline DualBuffer<T>::DualBuffer() : handle(nullptr), gpuBufferCapacity(0) {}

template <typename T>
inline DualBufferStatus
DualBuffer<T>::FlushToGPU(nvrhi::DeviceHandle device, nvrhi::CommandListHandle current_command_list)
{
	if (cpuMem.empty())
	{
		return DualBufferStatus::NO_UPLOAD_NEEDED;
	}

	DualBufferStatus status = EnsureGPUCapacity(device, cpuMem.size());

	/* A failed buffer allocation is the only error case. */
	if (status == DualBufferStatus::BUFFER_ALLOCATION_FAILED)
	{
		return status;
	}

	current_command_list->writeBuffer(handle, cpuMem.data(), cpuMem.size() * sizeof(T));

	return (status == DualBufferStatus::BUFFER_RESIZED) ? status : DualBufferStatus::UPLOAD_SUCCESS;
}

template <typename T>
inline DualBufferStatus DualBuffer<T>::EnsureGPUCapacity(nvrhi::DeviceHandle device, size_t required_capacity)
{
	if ((required_capacity <= gpuBufferCapacity))
	{
		return DualBufferStatus::NO_RESIZE_NEEDED;
	}

	size_t new_capacity = (gpuBufferCapacity > 0) ? gpuBufferCapacity : 1;

	while (new_capacity < required_capacity)
	{
		new_capacity *= 2;
	}

	if (increaseGPUBufferSize(device, new_capacity))
	{
		return DualBufferStatus::BUFFER_RESIZED;
	}

	return DualBufferStatus::BUFFER_ALLOCATION_FAILED;
}

template <typename T> inline bool DualBuffer<T>::increaseGPUBufferSize(nvrhi::DeviceHandle device, size_t new_size)
{
	nvrhi::BufferDesc	bufferDesc = nvrhi::BufferDesc()
										 .setByteSize(new_size * sizeof(T))
										 .enableAutomaticStateTracking(nvrhi::ResourceStates::ShaderResource)
										 .setStructStride(sizeof(T));
	nvrhi::BufferHandle new_buffer = device->createBuffer(bufferDesc);

	if (!new_buffer)
	{
		return false;
	}

	handle = new_buffer;
	gpuBufferCapacity = new_size;

	return true;
}
} // namespace Mupfel
