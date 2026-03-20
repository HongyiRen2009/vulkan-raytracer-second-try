#include "ChunkManager.h"
#include <iostream>
ivec3 ChunkManager::allChunksMin = ivec3(0);
ivec3 ChunkManager::allChunksMax = ivec3(0);
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
void ChunkManager::generateChunksAroundCamera(ivec3 cameraChunkPos, vector<uint32_t>& invalidChunkIndices) {
	for (int i = 0; i < chunks.size(); i++) {
		ivec3 chunkPos = chunks[i]->chunkPosition;
		int dx = abs(chunkPos.x - cameraChunkPos.x);
		int dz = abs(chunkPos.z - cameraChunkPos.z);

		if (dx > renderDistance || dz > renderDistance) {
			invalidChunkIndices.push_back(chunks[i]->hashTableSlot);
			unloadChunk(i);
			i--;
		}
	}

	// Load chunks around camera
	for (int x = -renderDistance; x <= renderDistance; x++) {
		for (int z = -renderDistance; z <= renderDistance; z++) {
			for (int y = 0; y <= renderDistanceHeight; y++) {
				ivec3 chunkPos = cameraChunkPos + ivec3(x, y, z);
				bool chunkExists = false;
				for (const auto& chunk : chunks) {
					if (chunk->chunkPosition == chunkPos) {
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

	allChunksMin = ivec3(
		(cameraChunkPos.x - renderDistance) * Chunk::CHUNK_SIZE,
		(cameraChunkPos.y - renderDistanceHeight) * Chunk::CHUNK_SIZE,
		(cameraChunkPos.z - renderDistance) * Chunk::CHUNK_SIZE
	);
	allChunksMax = ivec3(
		(cameraChunkPos.x + renderDistance + 1) * Chunk::CHUNK_SIZE,
		(cameraChunkPos.y + renderDistanceHeight + 1) * Chunk::CHUNK_SIZE,
		(cameraChunkPos.z + renderDistance + 1) * Chunk::CHUNK_SIZE
	);
}
uint32_t ChunkManager::hashChunkPos(ivec3 pos) {

	const uint p1 = 73856093u;
	const uint p2 = 19349663u;
	const uint p3 = 83492791u;

	uint hash = (uint(pos.x) * p1) ^ (uint(pos.y) * p2) ^ (uint(pos.z) * p3);

	hash ^= hash >> 16;
	hash *= 0x85ebca6bu;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35u;
	hash ^= hash >> 16;
	return hash % 10000u;

}
void ChunkManager::loadChunk(ivec3 spawnPosition) {
	unsigned int slot = popFreeSlot();
	auto chunk = std::make_unique<Chunk>(seed, spawnPosition);
	chunk->gpuSlot = slot;
	chunk->generateTerrain(&unloadedChunkBlockstoPlace);

	uint32_t hash = hashChunkPos(spawnPosition);
	uint32_t foundSlot = UINT32_MAX;

	for (int i = 0; i < 100; i++) {
		uint32_t index = (hash + i) % 10000;
		if (hashedChunkPositions.find(index) == hashedChunkPositions.end()) {
			chunk->hashTableSlot = index;
			hashedChunkPositions.insert(index);
			foundSlot = index;
			break;
		}
	}

	if (foundSlot != UINT32_MAX) {
		chunk->isDirty = true;
		chunks.push_back(std::move(chunk));
	}
	else {
		std::cout << "ERROR: No free hash slots available for chunk at " << spawnPosition.x << ", " << spawnPosition.y << ", " << spawnPosition.z << std::endl;
		freeChunkSlots.push(slot);  // Return the slot since we can't use the chunk
	}
}
void ChunkManager::unloadChunk(int index) {

	freeChunkSlots.push(chunks[index]->gpuSlot);
	hashedChunkPositions.erase(chunks[index]->hashTableSlot);
	chunks.erase(chunks.begin() + index);


}