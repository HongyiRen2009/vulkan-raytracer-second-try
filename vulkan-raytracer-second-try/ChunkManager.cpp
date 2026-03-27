#include "ChunkManager.h"
#include <iostream>
#include <string>
ivec3 ChunkManager::allChunksMin = ivec3(0);
ivec3 ChunkManager::allChunksMax = ivec3(0);
ChunkManager::ChunkManager() {
	chunkGenerationThread = std::thread(&ChunkManager::chunkGenerationWorker, this);
	chunkFreeList.init(256*1024*1024/sizeof(Node)); // 256 MB
}

ChunkManager::~ChunkManager() {
	{
		std::unique_lock<std::mutex> lock(chunkGenMutex);
		shouldShutdown = true;
	}
	chunkGenCV.notify_one();
	if (chunkGenerationThread.joinable()) {
		chunkGenerationThread.join();
	}
}
void ChunkManager::init(unsigned int seed) {
	this->seed = seed;
}
string getChunkSetString(ivec3 chunkPos) {
	return to_string(chunkPos.x) + "," + to_string(chunkPos.y) + "," + to_string(chunkPos.z);
}
void ChunkManager::generateChunksAroundCamera(ivec3 cameraChunkPos) {
	for (int i = 0; i < chunks.size(); i++) {
		ivec3 chunkPos = chunks[i]->chunkPosition;
		int dx = abs(chunkPos.x - cameraChunkPos.x);
		int dz = abs(chunkPos.z - cameraChunkPos.z);

		if (dx > renderDistance || dz > renderDistance) {
			invalidChunks.push_back(chunks[i]->hashTableSlot);
			unloadChunk(i);
			loadedChunkPositions.erase(getChunkSetString(chunkPos));
			i--;
		}
	}

	// Load chunks around camera
	vector<ivec3> newChunkPositions;
	for (int x = -renderDistance; x <= renderDistance; x++) {
		for (int z = -renderDistance; z <= renderDistance; z++) {
			for (int y = 0; y <= renderDistanceHeight; y++) {
				ivec3 chunkPos = cameraChunkPos + ivec3(x, y, z);
				if (loadedChunkPositions.find(getChunkSetString(chunkPos)) == loadedChunkPositions.end()) {
					newChunkPositions.push_back(chunkPos);
				}
			}
		}
	}
	loadChunks(newChunkPositions);
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

void ChunkManager::onChunkGenerationComplete() {
	pendingChunkGenerations--;
}
void ChunkManager::loadChunks(vector<ivec3> spawnPositions) {
	for (int i = 0;i < spawnPositions.size();i++) {
		auto chunk = std::make_unique<Chunk>(seed, spawnPositions[i]);
		uint32_t hash = hashChunkPos(spawnPositions[i]);
		uint32_t foundSlot = UINT32_MAX;
		loadedChunkPositions.insert(getChunkSetString(spawnPositions[i]));
		for (int j = 0; j < 100; j++) {
			uint32_t index = (hash + j) % 10000;
			if (hashedChunkPositions.find(index) == hashedChunkPositions.end()) {

				chunk->hashTableSlot = index;
				hashedChunkPositions.insert(index);
				foundSlot = index;
				break;
			}

		}

		if (foundSlot != UINT32_MAX) {
			chunk->isDirty = true;
			Chunk* chunkPtr = chunk.get();
			chunks.push_back(std::move(chunk));
			pendingChunkGenerations++;
			threadPool.enqueue([this, chunkPtr]() {
				chunkPtr->generateTerrain();
				chunkPtr->gpuSlot = chunkFreeList.allocate(chunkPtr->nodePool.size());
				onChunkGenerationComplete();
				});

		}
		else {
			std::cout << "ERROR: No free hash slots available for chunk at " << spawnPositions[i].x << ", " << spawnPositions[i].y << ", " << spawnPositions[i].z << std::endl;
		}
	}
}
void ChunkManager::requestChunkGeneration(ivec3 cameraChunkPos, bool blocking) {
	isFinishedGeneratingChunks = false;
	isGeneratingChunks = true;
	{
		std::unique_lock<std::mutex> lock(chunkGenMutex);
		chunkGenerationQueue.push(cameraChunkPos);

	}
	chunkGenCV.notify_one();

	if (blocking) {
		std::unique_lock<std::mutex> lock(chunkGenMutex);
		chunkGenCV.wait(lock, [this]() { return isFinishedGeneratingChunks; });
	}
}
void ChunkManager::chunkGenerationWorker() {
	while (true) {
		std::unique_lock<std::mutex> lock(chunkGenMutex);
		chunkGenCV.wait(lock, [this]() { return !chunkGenerationQueue.empty() || shouldShutdown; });

		if (shouldShutdown && chunkGenerationQueue.empty()) {
			break;
		}

		if (!chunkGenerationQueue.empty()) {
			ivec3 cameraChunkPos = chunkGenerationQueue.front();
			chunkGenerationQueue.pop();
			lock.unlock();

			generateChunksAroundCamera(cameraChunkPos);
			while (pendingChunkGenerations.load() > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			lock.lock();
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
			isFinishedGeneratingChunks = true;
			
			chunkGenCV.notify_all();
		}
	}
}
void ChunkManager::unloadChunk(int index) {
	chunkFreeList.deallocate(chunks[index]->gpuSlot, chunks[index]->nodePool.size());
	hashedChunkPositions.erase(chunks[index]->hashTableSlot);
	chunks.erase(chunks.begin() + index);


}