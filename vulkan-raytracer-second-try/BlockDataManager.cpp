#include "BlockDataManager.h"
enum BlockType {
	AIR = 0,
	GRASS = 1,
	DIRT = 2,
	STONE = 3,
	WOOD = 4,
	LEAF = 5,
	WATER = 6

};
struct blockUV {
	unsigned int blockBottomLefts[6];
	bool transparent = false;
	bool interactable = true;
};
blockUV grass = {
	{
4,
4,
4,
4,
		0, //Top
		1
	}
};
blockUV dirt = {
{
		1,
		1,
		1,
		1,
		1,
		1,
}
};
blockUV stone = {
{
		3,
		3,
		3,
		3,
		3,
		3,

}
};
blockUV wood = {
{
2,
2,
2,
2,
5,5
}
};
blockUV leaf = {
{
6,
6,
6,
6,
6,
6,


},
true
};
blockUV water = {
{
7,
7,
7,
7,
7,
7,
},
true,
false
};
blockUV sand = {
{
		8,
		8,
		8,
		8,
		8,
		8,

}
};
blockUV gravel = {
{
		9,
		9,
		9,
		9,
		9,
		9,

}
};
blockUV snow = {
{
		10,
		10,
		10,
		10,
		10,
		10,
}
};
blockUV blockUVs[9] = {
	grass,dirt,stone,wood,leaf,water,sand,gravel,snow
};
blockData BlockDataManager::blockDataArray[256];
// --- Chunk methods ---
int placed = 0;
int missing = 0;
BlockDataManager::BlockDataManager() {
}
BlockDataManager::~BlockDataManager() {
}
unsigned int BlockDataManager::maxBlockId() {
	return sizeof(blockUVs) / sizeof(blockUVs[0]);
}
void BlockDataManager::populateBlockDataArray() {
	for (int i = 0; i < 256; ++i) {
		blockDataArray[i] = {};
	}

	for (unsigned int i = 0; i < maxBlockId(); ++i) {
		blockData data{};
		data.face0123 = glm::uvec4(
			blockUVs[i].blockBottomLefts[0],
			blockUVs[i].blockBottomLefts[1],
			blockUVs[i].blockBottomLefts[2],
			blockUVs[i].blockBottomLefts[3]
		);
		data.face45pad = glm::uvec4(
			blockUVs[i].blockBottomLefts[4],
			blockUVs[i].blockBottomLefts[5],
			0u,
			0u
		);

		// voxel ID 0 = air, so block i maps to ID i+1
		blockDataArray[i + 1] = data;
	}
}
bool BlockDataManager::isInteractable(unsigned int blockId) {
	if (blockId == 0) return false; // Air is not interactable
	if (blockId > sizeof(blockUVs) / sizeof(blockUVs[0])) return false; // Invalid block ID
	return blockUVs[blockId - 1].interactable;
}