#pragma once

#include <vector>
#include <tuple>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>
#include <memory>
#include <functional> 
#include "ChunkTypes.h"
using namespace glm;
using namespace std;

class PerlinNoise;
struct IVec2Hash {
    std::size_t operator()(const glm::ivec2& v) const noexcept {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};

class Chunk {
public:

    static constexpr unsigned int CHUNK_SIZE = 16;
    static constexpr unsigned int CHUNK_HEIGHT = 256;
	static constexpr unsigned int CHUNK_VOXEL_SIZE_IN_BYTES = CHUNK_SIZE*CHUNK_SIZE*CHUNK_HEIGHT*sizeof(unsigned int);
    ivec2 chunkPosition;
    Chunk(unsigned int seed, ivec2 spawnPosition);
    void generateTerrain(vector<tuple<ivec3, unsigned int>>* unloadedChunkBlockstoPlace);
    ~Chunk();
    bool isDirty = true;
    unsigned int gpuSlot;
    unsigned int getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, unsigned int newBlock);
    static bool isInteractable(unsigned int blockId);
    static unsigned int maxBlockId();
    unsigned int blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]{};
	static ChunkTypes::blockData blockDataArray[256];
    static void populateBlockDataArray();
private:
    unsigned int generateTerrainHeight(int x, int z);
    bool generateTreeValid(int x, int y, int z, int layerSize);
    void generateTree(int x, int y, int z, vector<tuple<ivec3, unsigned int>>* unloadedChunkBlockstoPlace);
    PerlinNoise* perlinNoise;

};