#include "GPUAllocator.h"
#include "glad.h"
#include "raylib.h"
#include <vector>

using namespace Mupfel;

static constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

GPUAllocator::Handle GPUAllocator::allocateGPUBuffer(uint32_t size)
{
	Handle new_handle;
	glGenBuffers(1, &new_handle.id);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, new_handle.id);

	

	glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, flags);

	new_handle.mapped_ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, flags);

	if (!new_handle.mapped_ptr)
	{
		TraceLog(LOG_ERROR, "Allocation of %u bytes of Shader Storage Buffer failed...", size);
	}

	new_handle.capacity = size;

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		TraceLog(LOG_ERROR, "### GPU Allocator Error!", size);
	}

	return new_handle;
}

void GPUAllocator::freeGPUBuffer(GPUAllocator::Handle& h)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, h.id);
	
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glDeleteBuffers(1, &h.id);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		TraceLog(LOG_ERROR, "### GPU Allocator Error!");
	}

	h.id = 0;
	h.capacity = 0;
	h.mapped_ptr = nullptr;
}

void GPUAllocator::reallocateGPUBuffer(Handle& h, uint32_t new_size)
{
	if (h.capacity >= new_size)
	{
		return;
	}

	/* create a temporary copy of the contents */
	std::vector<uint8_t> tmp;
	uint8_t *buff = static_cast<uint8_t*>(h.mapped_ptr);

	for (uint32_t i = 0; i < h.capacity; i++)
	{
		tmp.push_back(buff[i]);
	}

	/* Free old Buffer */
	freeGPUBuffer(h);

	/* Allocate new Buffer */
	h = allocateGPUBuffer(new_size);

	/* Copy back the memory */
	buff = static_cast<uint8_t*>(h.mapped_ptr);

	for (uint32_t i = 0; i < tmp.size(); i++)
	{
		buff[i] = tmp[i];
	}
}

void Mupfel::GPUAllocator::MemBarrier()
{
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}
