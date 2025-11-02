#pragma once
#include "Entity.h"
#include <bitset>

namespace Mupfel {

	class Archetype {
	public:
		typedef std::bitset<32> Signature;
	public:
		Archetype(Entity::Signature sig, size_t in_handle) : comp_signature(sig), handle(in_handle), arch_signature() { arch_signature.set(in_handle); }
		size_t handle;
		Entity::Signature comp_signature;
		Signature arch_signature;
	};

}


