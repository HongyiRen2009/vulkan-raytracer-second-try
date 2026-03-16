#pragma once
#include <stack>
#include <vector>
#include "Chunk.h"
#include <set>
using namespace std;
class ChunkManager {
	public:
		ChunkManager();
		~ChunkManager();
		void init(unsigned int seed);
		const vector<unique_ptr<Chunk>>& GetChunks() {
			return chunks;
		}
		void generateChunksAroundCamera( ivec2 cameraChunkPos, vector<uint32_t>& invalidChunkIndices);
		uint hashChunkPos(ivec2 pos); 
	private:
		int renderDistance = 1;
	unsigned int seed;
	vector<unique_ptr<Chunk>> chunks;

	stack<unsigned int> freeChunkSlots;
	set<uint32_t> hashedChunkPositions;
	unsigned int currentChunkIndex = 0;
	unsigned int popFreeSlot();
	void unloadChunk(int index);
	void loadChunk( ivec2 spawnPosition);
	vector <tuple<ivec3, unsigned int>> unloadedChunkBlockstoPlace;

};