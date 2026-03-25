#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include "GPUVector.h"

namespace Mupfel
{
	class GPUPrefixSum
	{
	public:
		GPUPrefixSum();
		~GPUPrefixSum();
		void Init(uint32_t maxElements);
		void DeInit();
		void Run(GPUVector<uint32_t>& input, uint32_t element_count);
	private:
		bool InitComplete() const;
		void DispatchBlockScan(uint32_t dataBuffer, uint32_t blockSumBuffer, uint32_t numBlocks);
		void DispatchAddOffsets(uint32_t dataBuffer, uint32_t blockOffsetBuffer, uint32_t numBlocks);
	private:
		uint32_t prefix_sum_shader;
		uint32_t add_offsets_shader;
		std::vector<std::unique_ptr<GPUVector<uint32_t>>> blocksum_levels;
		std::unique_ptr<GPUVector<uint32_t>> number_of_elements;
	};
}



