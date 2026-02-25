#pragma once
#include <stack>
#include <vector>
#include "Chunk.h"
using namespace std;
class ChunkManager {
	public:
		ChunkManager();
		~ChunkManager();
		vector<Chunk> chunks;
		void loadChunk(unsigned int seed, ivec2 spawnPosition);
	private:
	stack<unsigned int> freeChunkSlots;
	unsigned int currentChunkIndex = 0;
	unsigned int popFreeSlot();
	void pushFreeSlot(unsigned int slot) {
		freeChunkSlots.push(slot);
	}
	vector <tuple<ivec3, unsigned int>> unloadedChunkBlockstoPlace;

};