#include "FreeList.h"
#include <iostream>

unsigned int FreeList::allocate(unsigned int size) {
	std::unique_lock<std::mutex> lock(mutex);
	for (int i = 0; i < (int)entries.size(); i++) {
		if (entries[i].size >= size) {
			unsigned int index = entries[i].index;
			entries[i].index += size;
			entries[i].size -= size;
			if (entries[i].size == 0) {
				entries.erase(entries.begin() + i);
			}
			validate();
			return index;
		}
	}
	throw std::runtime_error("Out of memory in FreeList!");
}

void FreeList::deallocate(unsigned int index, unsigned int size) {
	std::unique_lock<std::mutex> lock(mutex);

	auto it = entries.begin();
	while (it != entries.end() && it->index < index) {
		++it;
	}
	entries.insert(it, { index, size });

	for (int i = 1; i < (int)entries.size(); i++) {
		if (entries[i - 1].index + entries[i - 1].size == entries[i].index) {
			entries[i - 1].size += entries[i].size;
			entries.erase(entries.begin() + i);
			i--;
		}
	}
	validate();
}

bool FreeList::validate() {
	for (size_t i = 0; i + 1 < entries.size(); i++) {
		if (entries[i].index + entries[i].size > entries[i + 1].index) {
			std::cerr << "FreeList ERROR: Overlap detected at index " << i << std::endl;
			return false;
		}
		if (entries[i].index + entries[i].size == entries[i + 1].index) {
			std::cerr << "FreeList WARNING: Adjacent free blocks not coalesced at index " << i << std::endl;
			return false;
		}
	}
	return true;
}