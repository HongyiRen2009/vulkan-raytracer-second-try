#pragma once
#include <vector>
#include <stdexcept>
#include <mutex>
struct FreeListEntry
{
	unsigned int index;
	unsigned int size;
};
class FreeList {
	public:
	FreeList() {
	}
	~FreeList() {
	}
	void init(unsigned int totalSize) {
		entries.clear();
		entries.push_back({ 0, totalSize });
	}
	unsigned int allocate(unsigned int size);
	void deallocate(unsigned int index, unsigned int size);
	private:
		std::vector<FreeListEntry> entries;
		std::mutex mutex;
		bool validate();

};