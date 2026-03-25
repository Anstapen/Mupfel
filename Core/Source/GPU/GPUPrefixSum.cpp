#include "GPUPrefixSum.h"

#include "raylib.h"
#include "rlgl.h"
#include "glad.h"

using namespace Mupfel;

constexpr uint32_t BLOCK_THREADS = 256;
constexpr uint32_t ELEMENTS_PER_THREAD = 2;
constexpr uint32_t ELEMENTS_PER_BLOCK = BLOCK_THREADS * ELEMENTS_PER_THREAD;

Mupfel::GPUPrefixSum::GPUPrefixSum() :
	prefix_sum_shader(0), add_offsets_shader(0)
{
}

Mupfel::GPUPrefixSum::~GPUPrefixSum()
{
}

void Mupfel::GPUPrefixSum::Init(uint32_t maxElements)
{
	blocksum_levels.clear();

	uint32_t current = maxElements;

	while (true)
	{
		uint32_t numBlocks =
			(current + ELEMENTS_PER_BLOCK - 1) / ELEMENTS_PER_BLOCK;

		if (numBlocks <= 1)
			break;

		auto buffer = std::make_unique<GPUVector<uint32_t>>();
		buffer->resize(numBlocks, 0);

		blocksum_levels.push_back(std::move(buffer));

		current = numBlocks;
	}

	number_of_elements.reset(new GPUVector<uint32_t>());
	number_of_elements->resize(1, 0);
	number_of_elements->operator[](0) = maxElements;

	char* shader_code = LoadFileText("Shaders/blelloch_prefix_sum.glsl");
	int shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	prefix_sum_shader = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);

	shader_code = LoadFileText("Shaders/add_offsets.glsl");
	shader_data = rlCompileShader(shader_code, RL_COMPUTE_SHADER);
	add_offsets_shader = rlLoadComputeShaderProgram(shader_data);
	UnloadFileText(shader_code);
}

void GPUPrefixSum::DeInit()
{
	if (prefix_sum_shader != 0)
	{
		glDeleteProgram(prefix_sum_shader);
		prefix_sum_shader = 0;
	}
	if (add_offsets_shader != 0)
	{
		glDeleteProgram(add_offsets_shader);
		add_offsets_shader = 0;
	}
	blocksum_levels.clear();
}

void Mupfel::GPUPrefixSum::Run(GPUVector<uint32_t>& input, uint32_t element_count)
{
	if(!InitComplete())
	{
		return;
	}

	std::vector<uint32_t> levelSizes;

	uint32_t currentCount = element_count;
	uint32_t level = 0;

	// -----------------------------
	// Phase 1: DOWN-SWEEP (collect block sums)
	// -----------------------------
	while (true)
	{
		uint32_t numBlocks =
			(currentCount + ELEMENTS_PER_BLOCK - 1) / ELEMENTS_PER_BLOCK;

		if (numBlocks <= 1)
			break;

		levelSizes.push_back(numBlocks);

		number_of_elements->operator[](0) = currentCount;

		DispatchBlockScan(
			(level == 0) ? input.GetSSBOID()
			: blocksum_levels[level - 1]->GetSSBOID(),
			blocksum_levels[level]->GetSSBOID(),
			numBlocks
		);

		currentCount = numBlocks;
		level++;
	}

	// -----------------------------
	// Phase 2: scan top level (single block)
	// -----------------------------
	number_of_elements->operator[](0) = currentCount;

	DispatchBlockScan(
		(level == 0) ? input.GetSSBOID()
		: blocksum_levels[level - 1]->GetSSBOID(),
		0, // optional, kein Blocksum nötig
		1
	);

	// -----------------------------
	// Phase 3: UP-SWEEP (add offsets)
	// -----------------------------
	for (int i = level - 1; i >= 0; i--)
	{
		uint32_t numBlocks = levelSizes[i];

		number_of_elements->operator[](0) =
			(i == 0) ? element_count : levelSizes[i - 1];

		DispatchAddOffsets(
			(i == 0) ? input.GetSSBOID()
			: blocksum_levels[i - 1]->GetSSBOID(),
			blocksum_levels[i]->GetSSBOID(),
			numBlocks
		);
	}
}

bool Mupfel::GPUPrefixSum::InitComplete() const
{
	if(prefix_sum_shader == 0 || add_offsets_shader == 0)
	{
		return false;
	}

	return true;
}

void Mupfel::GPUPrefixSum::DispatchBlockScan(uint32_t dataBuffer, uint32_t blockSumBuffer, uint32_t numBlocks)
{
	glUseProgram(prefix_sum_shader);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, blockSumBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, number_of_elements->GetSSBOID());

	glDispatchCompute(numBlocks, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Mupfel::GPUPrefixSum::DispatchAddOffsets(uint32_t dataBuffer, uint32_t blockOffsetBuffer, uint32_t numBlocks)
{
	glUseProgram(add_offsets_shader);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, dataBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, blockOffsetBuffer);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, number_of_elements->GetSSBOID());

	glDispatchCompute(numBlocks, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
