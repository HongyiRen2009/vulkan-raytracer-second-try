#include "ChunkManager.h"
ChunkManager::ChunkManager() {
}
ChunkManager::~ChunkManager() {
}
void ChunkManager::init(unsigned int seed) {
	this->seed = seed;
}
unsigned int ChunkManager::popFreeSlot() {
	if (!freeChunkSlots.empty()) {
		unsigned int slot = freeChunkSlots.top();
		freeChunkSlots.pop();
		return slot;
	}
	return currentChunkIndex++;
}
void ChunkManager::generateChunksAroundCamera( ivec2 cameraChunkPos) {
	for (int x = -renderDistance; x <= renderDistance; x++) {
		for (int z = -renderDistance; z <= renderDistance; z++) {
			ivec2 chunkPos = cameraChunkPos + ivec2(x, z);
			bool chunkExists = false;
			for (const auto& chunk : chunks) {
				if (chunk.chunkPosition == chunkPos) {
					chunkExists = true;
					break;
				}
			}
			if (!chunkExists) {
				loadChunk(chunkPos);
			}
		}
	}
}
void ChunkManager::loadChunk(ivec2 spawnPosition) {
	unsigned int slot = popFreeSlot();
	Chunk chunk(seed, spawnPosition);
	chunk.gpuSlot = slot;
	chunk.generateTerrain(&unloadedChunkBlockstoPlace);
	chunks.push_back(chunk);

}