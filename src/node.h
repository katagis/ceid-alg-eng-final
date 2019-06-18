#ifndef __NODE_H_
#define __NODE_H_

#include "testbench.h"
AggregateTimer Timer;
#define NDEBUG

#include <array>
#include <utility>
#include <cassert>
#include <string>
#include <iostream>
#include <functional>

typedef unsigned int uint;

// 
// TODO list:
// 1. Change ValueType to non-ptr specific.
// 2. Implement GetRange and nextNode ptrs & iterators.
// 3. Use some kind of TreePath to optimise multiple finds.
// 

// PERF: check performance vs list
template<typename ArrayType, std::size_t ArraySize>
void insertAtArray(std::array<ArrayType, ArraySize>& arr, int lastIndex, int location, const ArrayType& elem) {
	assert(lastIndex < ArraySize);
	assert(location <= lastIndex);
	for (int i = lastIndex; i > location; --i) {
		arr[i] = std::move(arr[i - 1]);
	}
	arr[location] = elem;
}

template<typename ArrayType, std::size_t ArraySize>
void deleteFromArrayAt(std::array<ArrayType, ArraySize>& arr, int arrSize, int at) {
	assert(arrSize <= ArraySize);
	for (int i = at; i < arrSize - 1; ++i) {
		arr[i] = std::move(arr[i + 1]);
	}
}

template<typename ArrayType, std::size_t ArraySize>
int deleteFromArray(std::array<ArrayType, ArraySize>& arr, int arrSize, const ArrayType& elem) {
	assert(arrSize <= ArraySize);

	int foundIndex = 0;
	for (int i = 0; i < arrSize; ++i) {
		if (arr[i] == elem) {
			foundIndex = i;
			break;
		}
	}

	deleteFromArrayAt(arr, arrSize, foundIndex);
	return foundIndex;
}

template<typename KeyType, typename DataType, uint N>
struct Node {
	typedef std::pair<int, bool> ElemIndex;

	bool isLeaf;
	int childrenCount;
	Node* parent;
	// 2 seperate arrays for better cache management, since iterating keys only is frequent.
	std::array<KeyType, N> keys;
	std::array<Node*, N + 1> ptrs;

	// todo: remove this, it is only here to force the compiler to instanciate these templates
	// if not instanciated we cannot use them while debugging.
	void debug() {
		auto a = keys.data();
		auto b = ptrs.data();
	}

	int uid;

	// this does not resolve to actual struct wide static 
	// but static for this specific template instantiation
	static const int Parity = N % 2;
	static const int HN = N / 2 + Parity;

	Node()
		: isLeaf(false)
		, childrenCount(0)
		, parent(nullptr) {
		static uint total_uids = 0;
		uid = total_uids++;
		debug();
	}

	bool isRoot() const {
		return parent == nullptr;
	}

	DataType* getAsData(int index) {
		assert(isLeaf);
		assert(index <= childrenCount);
		return reinterpret_cast<DataType*>(ptrs[index - 1]);
	}

	void setAsData(int index, DataType* data) {
		assert(isLeaf);
		assert(index <= childrenCount);
		ptrs[index - 1] = reinterpret_cast<Node*>(data);
	}

	DataType* getNextLeaf() {
		assert(isLeaf);
		return ptrs[N];
	}

	// These 2 are roughly 70% of the total runtime since delete and insert use them too.
	// Expects found == false when called
	int getIndexOfFound(const KeyType& key, bool& found) const {
		if (childrenCount == 0 || key < keys[0]) { // leftmost as a special case
			return 0;
		}

		int left = 0;
		int right = childrenCount - 1;
		int middle = 0;

		while (left <= right) {
			middle = left + (right - left) / 2;

			if (key == keys[middle]) {
				found = true;
				return middle + 1;
			}

			if (key > keys[middle]) {
				left = middle + 1;
			}
			else {
				right = middle - 1;
			}
		}

		// when not found we still want to return the location
		return left;
	}

	int getIndexOf(const KeyType& key) const {

		// binary search here, with range from 0 -> childrenCount;
		//
		// example shape:
		//
		// |*| bcd |*| lay |*| --- |*|
		// 
		// we want to return the ptr index to follow to find the value
		// if key < bcd return 0
		// if key == bcd || key < lay return 1
		// ...

		if (childrenCount == 0 || key < keys[0]) { // leftmost as a special case
			return 0;
		}

		int left = 0;
		int right = childrenCount - 1;
		int middle = 0;

		while (left <= right) {
			middle = left + (right - left) / 2;
			
			if (key == keys[middle]) {
				return middle + 1;
			}

			if (key > keys[middle]) {
				left = middle + 1;
			}
			else {
				right = middle - 1;
			}
		}
		return left;
	}

	void insertAtLeaf(int index, const KeyType& key, DataType* data) {
		assert(getIndexOf(key).second == false); // This should not exist.
		assert(isRoot() || childrenCount + 1 >= N / 2);
		assert(isLeaf);

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount, index, reinterpret_cast<Node*>(data));
		childrenCount++;
	}

	void insertAtInternal(int index, const KeyType& key, Node* node) {
		assert(getIndexOf(key).second == false); // This should not exist.
		//assert(isRoot() || childrenCount + 1 >= N / 2);
		assert(!isLeaf);

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount + 1, index + 1, node);
		childrenCount++;
	}

	static Node* splitAndInsertLeaf(Node* initialNode, int insertIndex, const KeyType& key, DataType* data) {
		assert(initialNode->childrenCount == N);
	
		Node* rightNode = new Node();
		rightNode->isLeaf = true;
		if (insertIndex < HN) {
			// Our element is in the left node

			// PERF: split loops for better cache
			for (int i = N - 1; i >= HN - Parity; --i) {
				rightNode->keys[i - HN + Parity] = std::move(initialNode->keys[i]);
				rightNode->ptrs[i - HN + Parity] = initialNode->ptrs[i];
			}
			rightNode->childrenCount = HN;
			initialNode->childrenCount = HN - Parity;
			initialNode->insertAtLeaf(insertIndex, key, data);
		}
		else {

			// PERF: for now we do the same as above,
			// this moves some items 2 times and can be optimised
			
			for (int i = N - 1; i >= HN; --i) {
				rightNode->keys[i - HN] = std::move(initialNode->keys[i]);
				rightNode->ptrs[i - HN] = initialNode->ptrs[i];
			}
			rightNode->childrenCount = HN - Parity;
			initialNode->childrenCount = HN;
			rightNode->insertAtLeaf(insertIndex - HN, key, data);
		}

		return rightNode;
	}

	// return "popped" key, the one that gets lost from the split
	static KeyType splitAndInsertInternal(Node* initialNode, Node*& outNewNode, int insertIndex, const KeyType& key, Node* ptrInsert) {
		assert(initialNode->childrenCount == N);
		assert(ptrInsert);

		KeyType poppedKey;
		outNewNode = new Node();

		if (insertIndex < HN) {

			// Our element is in the left node

			// PERF: split loops for better cache
			for (int i = N; i > HN; --i) {
				outNewNode->ptrs[i - HN] = initialNode->ptrs[i];
				outNewNode->keys[i - HN - 1] = std::move(initialNode->keys[i - 1]);
			}
			outNewNode->ptrs[0] = initialNode->ptrs[HN];
			poppedKey = initialNode->keys[HN - 1];

			outNewNode->childrenCount = HN - Parity;
			initialNode->childrenCount = HN - 1;

			initialNode->insertAtInternal(insertIndex, key, ptrInsert);

			ptrInsert->parent = initialNode;
		}
		else if (insertIndex == HN) {

			for (int i = N; i > HN; --i) {
				outNewNode->ptrs[i - HN] = initialNode->ptrs[i];
				outNewNode->keys[i - HN - 1] = std::move(initialNode->keys[i - 1]);
			}
			outNewNode->ptrs[0] = ptrInsert;
			poppedKey = key;

			outNewNode->childrenCount = HN - Parity;
			initialNode->childrenCount = HN;
		}
		else {
			// PERF: for now we do the same as above,
			// this moves some items 2 times and can be optimised

			// PERF: split loops for better cache
			for (int i = N; i - 1 > HN; --i) {
				outNewNode->ptrs[i - 1 - HN] = initialNode->ptrs[i];
				outNewNode->keys[i - 1 - HN - 1] = std::move(initialNode->keys[i - 1]);
			}
			outNewNode->ptrs[0] = initialNode->ptrs[HN + 1];
			poppedKey = initialNode->keys[HN];

			outNewNode->childrenCount = HN - 1 - Parity;
			initialNode->childrenCount = HN;

			outNewNode->insertAtInternal(insertIndex - HN - 1, key, ptrInsert);
		}
		assert(outNewNode->childrenCount >= N / 2);
		for (int i = 0; i < outNewNode->childrenCount + 1; ++i) {
			outNewNode->ptrs[i]->parent = outNewNode;
		}

		return std::move(poppedKey);
	}

	
	// Returns key index that was deleted
	int deleteKeyAndPtr(const KeyType& key, Node* ptr) {
		// PERF: Delete: 24ms. 
		// Can leave empty stuff and fix it during later stages (eg redistribution or even node delete)
		const int pos = deleteFromArray(keys, childrenCount, key);
		deleteFromArray(ptrs, childrenCount + !isLeaf, ptr);
		childrenCount--;
		return pos;
	}

	void moveInfoInplaceInternal(int rangeStart, int rangeEnd, int offset, const KeyType& leftKey) {
		assert(!isLeaf);
		assert(rangeEnd <= N);
		assert(rangeEnd + offset <= N);
		assert(rangeStart + offset >= 0);
		assert(offset > 0);

		for (int i = rangeEnd + offset; i >= rangeStart + offset; --i) {
			if (i - offset - 1 >= 0) {
				keys[i - 1] = keys[i - offset - 1];
			}
			else {
				keys[i - 1] = leftKey;
			}
			ptrs[i] = ptrs[i - offset];
		}
	}

	void moveInfoInplaceLeaf(int rangeStart, int rangeEnd, int offset) {
		assert(rangeEnd < N);
		assert(rangeEnd + offset < N);
		assert(rangeStart + offset >= 0);
		assert(offset > 0);

		for (int i = rangeEnd + offset; i >= rangeStart + offset; --i) {
			keys[i] = keys[i - offset];
			ptrs[i] = ptrs[i - offset];
		}
	}
};


#endif // __NODE_H_