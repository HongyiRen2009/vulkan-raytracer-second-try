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
struct TempNode {
    uint64_t mask;
    uint32_t blockId;
	uint32_t childIndices[64];
	uint16_t childCount;
};

struct Node {
    uint64_t mask;
    uint32_t blockId;
    uint32_t firstChildIndex;
};
class Chunk {
public:
    vector<Node> nodePool;
    static constexpr unsigned int CHUNK_SIZE = 64;
	static constexpr unsigned int CHUNK_VOXEL_SIZE_IN_BYTES = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(unsigned int); //Currently oversized
    unsigned int hashTableSlot;
    ivec3 chunkPosition;
	vector<tuple<ivec3, unsigned int>> unloadedChunkBlockstoPlace;
    Chunk(unsigned int seed, ivec3 spawnPosition);
    void generateTerrain();
    ~Chunk();
    bool isDirty = false;
    unsigned int gpuSlot;
    unsigned int getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, unsigned int newBlock);
    static bool isInteractable(unsigned int blockId);
    static unsigned int maxBlockId();
    unsigned int blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE]{};
	unsigned int zCurveBlocks[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE]{};
	static ChunkTypes::blockData blockDataArray[256];
    static void populateBlockDataArray();
private:
    unsigned int generateTerrainHeight(int x, int z);
    bool generateTreeValid(int x, int y, int z, int layerSize);
    void generateTree(int x, int y, int z);
    PerlinNoise* perlinNoise;
    void BuildTree(int x, int y, int z, int size);
    TempNode CreateNode(int x, int y, int z, int size);
	bool isRegionUniform(int x, int y, int z, int depth, unsigned int blockId);
    bool isRegion64UniformAVX(int x, int y, int z, unsigned int blockId);
    vector<TempNode> childNodePool;

    template<int SIZE>
    bool isRegionUniformTemplated(int x, int y, int z, unsigned int blockId);

};