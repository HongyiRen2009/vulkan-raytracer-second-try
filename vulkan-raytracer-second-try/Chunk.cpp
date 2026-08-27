#include <GLFW/glfw3.h>
#include "chunk.h"
#include <random>
#include <numeric>
#include <algorithm>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <tuple>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <queue>
#include <array>
#include <vector>
#include <mutex>
#include <utility>
#include <iostream>
#include <thread>
#include <chrono>
#include "include/libmorton/morton.h"
using namespace std;
using namespace glm;
unsigned int waterLevel = 130;
unsigned int mountainLevel = 200;
unsigned int snowLevel = 200;
// --- PerlinNoise class (can be moved to its own file if desired) ---
Chunk::Chunk(unsigned int seed, ivec3 spawnPosition)
{
	this->seed = seed;
	chunkPosition = spawnPosition;
}
bool Chunk::seededRandom(int x, int y, int chance) {
	unsigned int hash = x * 73856093 ^ y * 19349663 ^ seed * 83492791;
	hash = (hash ^ (hash >> 13)) * 0x5bd1e995;
	return (hash % 100) < chance;
	}
unsigned int  Chunk::generateTerrainHeight(int x, int z, FastNoiseLite* noise) {
	constexpr float minHeight = 40.0f;
	constexpr float maxHeight = 220.0f;
	float noiseValue = noise->GetNoise((float)x, (float)z);
	// FBm is centered around zero. The old fourth-power ridge mapping turns
	// FBm's midpoint into 1/16 of maxHeight, placing almost everything below
	// the water level. Remap the bounded [-1, 1] result linearly instead.
	float normalizedHeight = glm::clamp(noiseValue * 0.5f + 0.5f, 0.0f, 1.0f);
	unsigned int terrainHeight = static_cast<unsigned int>(glm::mix(minHeight, maxHeight, normalizedHeight));
	return terrainHeight;
}

bool Chunk::generateTreeValid(int x, int y, int z, int layerSize) {
	for (int i = 1;i <= 5;i++) {
		if (x < 0 || x >= CHUNK_SIZE || y + i < 0 || y + i >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE || getBlock(x, y + i, z) != 0) {
			return false;
		}
	}
	for (int i = 5;i < 9;i++) {
		for (int j = -layerSize;j <= layerSize;j++) {
			for (int k = -layerSize;k <= layerSize;k++) {
				if (x + j < 0 || x + j >= CHUNK_SIZE || y + i < 0 || y + i >= CHUNK_SIZE || z + k < 0 || z + k >= CHUNK_SIZE || getBlock(x + j, y + i, z + k) != 0) {
					return false;
				}
			}
		}
	}
	return true;
}

void Chunk::setBlock(int x, int y, int z, unsigned int newBlock) {
	if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
		return;
	}
	blocks[x][y][z] = newBlock;
}
unsigned int Chunk::getBlock(int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
		return 0;
	}
	return blocks[x][y][z];
}
void Chunk::generateTree(int x, int y, int z) {
	int layerSize = 2;

	if (!generateTreeValid(x, y, z, layerSize)) {
		return;
	}
	for (int i = 1;i < 9;i++) {
		if (i < 7) {
			setBlock(x, y + i, z, 4);
		}
		if (i < 5) {
			continue;
		}
		layerSize -= floor((i - 5) / 2);
		for (int j = -layerSize;j <= layerSize;j++) {
			for (int k = -layerSize;k <= layerSize;k++) {
				// Calculate distance from trunk center
				float dist = sqrt(j * j + k * k + (i - 7) * (i - 7)); // (i-7) centers the sphere at the top

				// Set base probability (e.g., 30% at the edge, 0% at the trunk)
				float maxDist = static_cast<float>(layerSize);
				float probability = (dist / maxDist) * 10.0f; // 0% at trunk, up to 60% at edge

				if (rand() % 100 < probability && abs(std::min(j, k)) > 1) {
					continue;
				}
				if (j == 0 && k == 0 && i < 7) {
					continue;
				}
				if (x + j < 0 || x + j >= CHUNK_SIZE || z + k < 0 || z + k >= CHUNK_SIZE) {
					
						unloadedChunkBlockstoPlace.push_back({
							ivec3(x + j, y + i, z + k) + ivec3(chunkPosition.x * CHUNK_SIZE, chunkPosition.y*CHUNK_SIZE, chunkPosition.z * CHUNK_SIZE),
							5
							});
				
					continue;
				}
				setBlock(x + j, y + i, z + k, 5);
			}
		}
	}
}
void Chunk::generateTerrain(FastNoiseLite* noise) {
	std::vector<std::vector<unsigned int>> columnHeights(CHUNK_SIZE, std::vector<unsigned int>(CHUNK_SIZE));
	for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
		for (unsigned int k = 0; k < CHUNK_SIZE; k++) {
			ivec2 globalPosition = ivec2(i + chunkPosition.x * CHUNK_SIZE, k + chunkPosition.z * CHUNK_SIZE);
			columnHeights[i][k] = generateTerrainHeight(globalPosition.x, globalPosition.y, noise);
		}
	}

	for (unsigned int j = 0; j < CHUNK_SIZE; j++) {
		for (unsigned int k = 0; k < CHUNK_SIZE; k++) {
			for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
				unsigned int globalY = j + chunkPosition.y * CHUNK_SIZE;
				unsigned int terrainHeight = columnHeights[i][k];
				
				if (blocks[i][j][k] != 0) continue;
				if (globalY > terrainHeight) {
					blocks[i][j][k] = (globalY <= waterLevel) ? BlockType::WATER : BlockType::AIR;
					continue;
				}
				if (globalY > snowLevel) {
					blocks[i][j][k] = BlockType::SNOW;
					continue;
				}
				if (globalY > mountainLevel) {
					blocks[i][j][k] = BlockType::STONE;
					continue;
				}


				if (terrainHeight < waterLevel + 2 && j > terrainHeight - 3 && terrainHeight >= waterLevel) {
					blocks[i][j][k] = BlockType::SAND;
					continue;
				}
				if (globalY == terrainHeight) {
					blocks[i][j][k] = (globalY >= waterLevel) ? BlockType::GRASS : BlockType::GRAVEL;
					continue;
				}
				if (globalY > terrainHeight - 6) {
					blocks[i][j][k] = (terrainHeight >= waterLevel) ? BlockType::DIRT : BlockType::GRAVEL;
					continue;
				}
				blocks[i][j][k] = 3;
			}
		}
	}
	for (unsigned int i = 0; i < CHUNK_SIZE; i++) {
		for (unsigned int k = 0; k < CHUNK_SIZE; k++) {
			ivec2 globalPosition = ivec2(i + chunkPosition.x * CHUNK_SIZE, k + chunkPosition.z * CHUNK_SIZE);
			unsigned int terrainHeight = columnHeights[i][k];

			int localTerrainHeight = (int)terrainHeight - (int)(chunkPosition.y * CHUNK_SIZE);

			if (localTerrainHeight < 0 || localTerrainHeight >= CHUNK_SIZE) {
				continue;
			}

			if (terrainHeight >= waterLevel && seededRandom(globalPosition.x, globalPosition.y, 2) && blocks[i][localTerrainHeight][k] == 1) {
				generateTree(i, localTerrainHeight, k);
			}
		}
	}
	BuildTree(0, 0, 0, CHUNK_SIZE);
}

TempNode Chunk::CreateNode(int x, int y, int z, int size) {
	TempNode node = {
		.mask = 0,
		.blockId = 0,
		.childIndices = {},
		.childCount = 0,
		.size = (uint8_t)size
	};

	if (size <= 1) {
		node.blockId = blocks[x][y][z];
		node.mask = (node.blockId == 0) ? 0ull : 1ull;
		return node;
	}

	const unsigned int baseBlock = blocks[x][y][z];
	if (isRegionUniform(x, y, z, size, baseBlock)) {
		node.blockId = baseBlock;
		node.mask = (baseBlock == 0u) ? 0ull : 0xFFFFFFFFFFFFFFFFull;
		node.childCount = 0;
		return node;
	}

	int childCount = 0;
	if (size == 4) {
		node.childIndices[0] = (uint32_t)oneNodePool.size();
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				TempNode childNode = CreateNode(
					x + (size / 4 * i),
					y + (size / 4 * j),
					z + (size / 4 * k),
					size / 4
				);

				if (childNode.mask == 0) {
					continue;
				}
				node.mask |= (1ull << (i * 16 + j * 4 + k));

				if (size == 4) {
					oneNodePool.push_back(childNode.blockId);
					continue;
				}
				childNodePool.push_back(childNode);
				node.childIndices[childCount++] = (uint32_t)(childNodePool.size() - 1);
			}
		}
	}

	node.childCount = (uint16_t)childCount;
	return node;
}
bool Chunk::isRegionUniform(int x, int y, int z, int depth, unsigned int blockId) {
	for (int i = 0; i < depth; i++) {
		for (int j = 0; j < depth; j++) {
			for (int k = 0; k < depth; k++) {
				if (blocks[x + i][y + j][z + k] != blockId) {
					return false;
				}
			}
		}
	}
	return true;
}
void Chunk::BuildTree(int x, int y, int z, int size) {
	TempNode rootNode = CreateNode(x, y, z, size);
	queue<pair<TempNode*, uint32_t>> nodeQueue;
	nodeQueue.push({ &rootNode, UINT32_MAX });

	while (!nodeQueue.empty()) {
		auto [currentNode, parentIdx] = nodeQueue.front();
		nodeQueue.pop();

		uint32_t currentIdx = (uint32_t)nodePool.size();
		if (parentIdx != UINT32_MAX) {
			nodePool[parentIdx].firstChildIndex = currentIdx;
		}

		Node node = {
			.mask = currentNode->mask,
			.blockId = currentNode->blockId,
			.firstChildIndex = 0
		};
		nodePool.push_back(node);

		if (currentNode->size == 4) {
			nodePool[currentIdx].firstChildIndex = currentNode->childIndices[0];
			continue;
		}

		if (currentNode->mask != 0 && currentNode->blockId == 0) {
			for (int i = 0; i < currentNode->childCount; i++) {
				nodeQueue.push({ &childNodePool[currentNode->childIndices[i]], i == 0 ? currentIdx : UINT32_MAX });
			}
		}
	}

	childNodePool.clear();
}
Chunk::~Chunk() {
}
