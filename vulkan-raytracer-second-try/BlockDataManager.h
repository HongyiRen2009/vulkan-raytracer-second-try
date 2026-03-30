#pragma once
#include <glm/glm.hpp>
struct alignas(16) blockData {
	glm::uvec4 face0123{ 0u, 0u, 0u, 0u }; // +Z, -Z, -X, +X
	glm::uvec4 face45pad{ 0u, 0u, 0u, 0u }; // +Y, -Y, pad, pad
};
class BlockDataManager {
public:
	BlockDataManager();
	~BlockDataManager();
	static bool isInteractable(unsigned int blockId);
	static unsigned int maxBlockId();
	static blockData blockDataArray[256];
	static void populateBlockDataArray();
};