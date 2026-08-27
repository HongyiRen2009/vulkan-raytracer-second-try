#pragma once

#include <vector>
#include <tuple>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>
#include <memory>
#include <functional> 
#include "include/FastNoiseLite.h"
#include "BlockDataManager.h"
using namespace glm;
using namespace std;

struct IVec2Hash {
    std::size_t operator()(const glm::ivec2& v) const noexcept {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
struct TempNode {
    uint64_t mask;
    uint32_t blockId;
	uint32_t childIndices[64];
	uint16_t childCount;
    uint8_t size;
};

struct Node {
    uint64_t mask;
    uint32_t blockId;
    uint32_t firstChildIndex;
};
class Chunk {
public:
    vector<Node> nodePool;
    vector<unsigned int> oneNodePool;
    static constexpr unsigned int CHUNK_SIZE = 64;
	static constexpr unsigned int CHUNK_VOXEL_SIZE_IN_BYTES = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(unsigned int); //Currently oversized
    unsigned int hashTableSlot;
    ivec3 chunkPosition;
	vector<tuple<ivec3, unsigned int>> unloadedChunkBlockstoPlace;
    Chunk(unsigned int seed, ivec3 spawnPosition);
    void generateTerrain(FastNoiseLite* noise);
    ~Chunk();
    bool isDirty = false;
    unsigned int gpuSlot = UINT32_MAX;
	unsigned int oneNodeSlot = UINT32_MAX;
    unsigned int getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, unsigned int newBlock);

    unsigned int blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE]{};


private:
    unsigned int generateTerrainHeight(int x, int z, FastNoiseLite* noise);
    bool generateTreeValid(int x, int y, int z, int layerSize);
    void generateTree(int x, int y, int z);
    void BuildTree(int x, int y, int z, int size);
    TempNode CreateNode(int x, int y, int z, int size);
	bool isRegionUniform(int x, int y, int z, int depth, unsigned int blockId);
    vector<TempNode> childNodePool;
	unsigned int seed;
    bool seededRandom(int x, int y, int chance);
};
