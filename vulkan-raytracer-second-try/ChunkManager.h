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
		vector<Chunk>& GetChunks() {
			return chunks;
		}
		void generateChunksAroundCamera( ivec2 cameraChunkPos);
		uint32_t hashChunkPos(ivec2 pos) {
			uint32_t hash = uint32_t(pos.x * 73856093) ^ uint32_t(pos.y * 19349663);
			for (int i = 0;i < 1000;i++) {
				if(hashedChunkPositions.find(hash) != hashedChunkPositions.end()) {
					hash += 1;
				}
				else {
					break;
				}
			}
			hashedChunkPositions.insert(hash);
			return hash % 1000u;
		}
	private:
		int renderDistance = 4;
	unsigned int seed;
	vector<Chunk> chunks;

	stack<unsigned int> freeChunkSlots;
	set<uint32_t> hashedChunkPositions;
	unsigned int currentChunkIndex = 0;
	unsigned int popFreeSlot();
	void unloadChunk(Chunk* chunk) {
		freeChunkSlots.push(chunk->gpuSlot);
		hashedChunkPositions.erase(hashChunkPos(chunk->chunkPosition));
		chunks.erase(remove_if(chunks.begin(), chunks.end(), [chunk](const Chunk& c) {
			return c.chunkPosition == chunk->chunkPosition;
			}), chunks.end());
		delete chunk;
	}
	void loadChunk( ivec2 spawnPosition);
	vector <tuple<ivec3, unsigned int>> unloadedChunkBlockstoPlace;

};