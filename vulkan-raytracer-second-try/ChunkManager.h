#pragma once
#include <stack>
#include <vector>
#include "Chunk.h"
#include "include/thread_pool/thread_pool.h"
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <queue>
#include <condition_variable>
#include "FreeList.h"
using namespace std;

class ChunkManager {
public:
	ChunkManager();
	~ChunkManager();
	void init(unsigned int seed);
	const vector<unique_ptr<Chunk>>& GetChunks() {
		return chunks;
	}
	uint hashChunkPos(ivec3 pos);
	static ivec3 allChunksMin;
	static ivec3 allChunksMax;
	bool isGeneratingChunks = false;
	bool isFinishedGeneratingChunks = false;
	std::vector<uint32_t> invalidChunks;
	int getRenderDistance() const { return renderDistance; }
	int getRenderDistanceHeight() const { return renderDistanceHeight; }
	void setRenderDistance(int distance) { renderDistance = distance; }
	void setRenderDistanceHeight(int height) { renderDistanceHeight = height; }
	void requestChunkGeneration(ivec3 cameraChunkPos, bool blocking = false);

private:
	std::mutex invalidChunksMutex;
	int renderDistance = 16;
	int renderDistanceHeight = 1;
	unsigned int seed;
	vector<unique_ptr<Chunk>> chunks;
	unordered_set<uint32_t> hashedChunkPositions;
	static inline dp::thread_pool<> threadPool{ 4 };
	unsigned int currentChunkIndex = 0;
	std::atomic<int> pendingChunkGenerations = 0;
	std::mutex pendingGenMutex;
	std::condition_variable pendingGenCV;

	unordered_set<string> loadedChunkPositions;
	void unloadChunk(int index);
	void loadChunks(vector<ivec3> spawnPositions);
	void onChunkGenerationComplete();
	std::thread chunkGenerationThread;
	std::mutex chunkGenMutex;

	std::condition_variable chunkGenCV;
	std::atomic<bool> shouldShutdown = false;
	std::queue<ivec3> chunkGenerationQueue;
	void chunkGenerationWorker();
	void generateChunksAroundCamera(ivec3 cameraChunkPos);
	FreeList chunkFreeList;
	FreeList oneNodeFreeList;
};