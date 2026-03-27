#include "FreeList.h"
#include <iostream>
unsigned int FreeList::allocate(unsigned int size) {
	std::unique_lock<std::mutex> lock(mutex);
	for (int i = 0;i < entries.size(); i++) {
		if(entries[i].size >= size) {
			unsigned int index = entries[i].index;
			entries[i].index += size;
			entries[i].size -= size;
			if(entries[i].size == 0) {
				entries.erase(entries.begin() + i);
			}
			validate();
			return index;
		}
	}
	throw std::runtime_error("Out of memory in FreeList!");
}
void FreeList::deallocate(unsigned int index, unsigned int size) {
	// Insert new entry in sorted position
	auto it = entries.begin();
	while (it != entries.end() && it->index < index) {
		++it;
	}
	entries.insert(it, { index, size });

	// Coalesce with both neighbors
	for (int i = 1; i < (int)entries.size(); i++) {
		// Merge if previous block is adjacent
		if (entries[i - 1].index + entries[i - 1].size == entries[i].index) {
			entries[i - 1].size += entries[i].size;
			entries.erase(entries.begin() + i);
			i--; // Check if this newly merged block can coalesce further
		}
	}
	validate();
}
bool FreeList::validate() {
	for (size_t i = 0; i < entries.size() - 1; i++) {
		// Check for overlaps
		if (entries[i].index + entries[i].size > entries[i + 1].index) {
			std::cerr << "FreeList ERROR: Overlap detected at index " << i << std::endl;
			return false;
		}
		// Check for adjacent blocks that should've coalesced
		if (entries[i].index + entries[i].size == entries[i + 1].index) {
			std::cerr << "FreeList WARNING: Adjacent free blocks not coalesced at index " << i << std::endl;
			return false;
		}
	}
	return true;
}