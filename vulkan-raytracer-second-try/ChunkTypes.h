#pragma once
#include <cstdint>
namespace ChunkTypes {
	struct alignas(16) blockData {
		glm::uvec4 face0123{ 0u, 0u, 0u, 0u }; // +Z, -Z, -X, +X
		glm::uvec4 face45pad{ 0u, 0u, 0u, 0u }; // +Y, -Y, pad, pad
	};
}