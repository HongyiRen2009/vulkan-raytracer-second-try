#include "ChunkManager.h"
ChunkManager::ChunkManager() {
}
ChunkManager::~ChunkManager() {
}
unsigned int ChunkManager::popFreeSlot() {
	if (!freeChunkSlots.empty()) {
		unsigned int slot = freeChunkSlots.top();
		freeChunkSlots.pop();
		return slot;
	}
	return currentChunkIndex++;
}
void ChunkManager::loadChunk(unsigned int seed, ivec2 spawnPosition) {
	unsigned int slot = popFreeSlot();
	Chunk chunk(seed, spawnPosition);
	chunk.gpuSlot = slot;
	chunk.generateTerrain(&unloadedChunkBlockstoPlace);
	chunks.push_back(chunk);

}