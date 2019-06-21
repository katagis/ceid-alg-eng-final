#ifndef __NODE_H_
#define __NODE_H_

#define NDEBUG
#include <array>
#include <utility>
#include <cassert>
#include <string>
#include <iostream>
#include <functional>

#ifndef INCR_BLOCKS
#define INCR_BLOCKS() do{ }while(0)
#endif

typedef unsigned int uint;

//
// TODO:
// Move next to ptrs[0] instead of ptrs[N]
// Check performance for std::string / comparison operator keys
//

#define MoveVal(expression) std::move(expression)

template<typename ArrayType, std::size_t ArraySize>
void insertAtArray(std::array<ArrayType, ArraySize>& arr, int lastIndex, int location, const ArrayType& elem) {
	assert(lastIndex < ArraySize);
	assert(location <= lastIndex);
	for (int i = lastIndex; i > location; --i) {
		arr[i] = MoveVal(arr[i - 1]);
	}
	arr[location] = elem;
}

template<typename ArrayType, std::size_t ArraySize>
void deleteFromArrayAt(std::array<ArrayType, ArraySize>& arr, int arrSize, int at) {
	assert(arrSize <= ArraySize);
	for (int i = at; i < arrSize - 1; ++i) {
		arr[i] = MoveVal(arr[i + 1]);
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

	// Order of members is important here for performance.
	int childrenCount;
	bool isLeaf;
	Node* parent;
	// 2 seperate arrays for better cache management, since iterating only keys is frequent.
	std::array<KeyType, N> keys;
	std::array<Node*, N + 1> ptrs;
	
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
		setNextLeaf(nullptr);
		INCR_BLOCKS();
	}

	bool isRoot() const {
		return parent == nullptr;
	}

	DataType* getAsData(int index) const {
		assert(isLeaf);
		assert(index <= childrenCount);
		return reinterpret_cast<DataType*>(ptrs[index]);
	}

	DataType*& getAsDataMutable(int index) {
		assert(isLeaf);
		assert(index <= childrenCount);
		return reinterpret_cast<DataType*&>(ptrs[index]);
	}

	void setAsData(int index, DataType* data) {
		assert(isLeaf);
		assert(index <= childrenCount);
		ptrs[index] = reinterpret_cast<Node*>(data);
	}

	Node* getNextLeaf() const {
		assert(isLeaf);
		return ptrs[N];
	}

	void setNextLeaf(Node* ptr) {
		assert(isLeaf);
		ptrs[N] = ptr;
	}
	
	//
	// This binary search is the part of the code that needs the most performance.
	// In our test these 2 functions run 19 million times and the body of the while loop runs about 200 million times. 
	// 
	// This specific implementation gives us the best performance on x64 because it compiles to a single cmp command.
	// The improvement over 2 cmp commands was about 10% of the time.
	// The compiled x64 code of the branch compiles (for KeyType == int) as 2 assembly commands: cmp, cmovg (conditional move if greater)
	// without any jump. (Verfied using Compiler Explorer at https //godbolt.org/)
	//

	// Expects found == false when called
	int getIndexOfFound(const KeyType& key, bool& found) const {

		// search from 0 -> childrenCount return ptr index to follow
		// example:
		//
		// |*| bcd |*| lay |*| --- |*|
		// 
		// we want to return the ptr index to follow to find the value
		// if key < 'bcd' return 0
		// if key == 'bcd' || key < 'lay' return 1
		// ...

		if (childrenCount == 0 || key < keys[0]) { 
			// special case for keys < keys[0] because it is a common case in the context of B+ tree.
			// provides actual benchmarked performance improvements
			return 0;
		}

		uint left = 0;
		uint middle = 0;
		uint half;
		uint len = childrenCount;

		while ((half = len / 2) > 0) {
			middle = left + half;
			if (keys[middle] <= key) {
				left = middle;
			}
			len -= half;
		}
		found = key == keys[left];
		return left + 1;
	}

	// Exact same as above without returning if found.
	// we usually dont really care if the key was found or not unless we are on a leaf.
	// This version is slightly faster than the above
	int getIndexOf(const KeyType& key) const {

		if (childrenCount == 0 || key < keys[0]) {
			return 0;
		}

		uint left = 0;
		uint middle = 0;
		uint half;
		uint len = childrenCount;

		while ((half = len / 2) > 0) {
			middle = left + half;
			if (keys[middle] <= key) {
				left = middle;
			}
			len -= half;
		}
		return left + 1;
	}

	bool isLeftMost(const KeyType& key) const {
		return key < keys[0];
	}

	bool isRightMost(const KeyType& key) const {
		return key >= keys[childrenCount - 1];
	}

	void insertAtLeaf(int index, const KeyType& key, DataType* data) {
		assert(isRoot() || childrenCount + 1 >= N / 2);
		assert(isLeaf);

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount, index, reinterpret_cast<Node*>(data));
		childrenCount++;
	}

	void insertAtInternal(int index, const KeyType& key, Node* node) {
		assert(!isLeaf);

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount + 1, index + 1, node);
		childrenCount++;
	}

	static Node* splitAndInsertLeaf(Node* initialNode, int insertIndex, const KeyType& key, DataType* data) {
		assert(initialNode->childrenCount == N);
		

		Node* rightNode = new Node();
		rightNode->isLeaf = true;
		rightNode->setNextLeaf(initialNode->getNextLeaf());
		initialNode->setNextLeaf(rightNode);

		if (insertIndex < HN) {
			// Our element is in the left node

			for (int i = N - 1; i >= HN - Parity; --i) {
				rightNode->keys[i - HN + Parity] = MoveVal(initialNode->keys[i]);
				rightNode->ptrs[i - HN + Parity] = initialNode->ptrs[i];
			}
			rightNode->childrenCount = HN;
			initialNode->childrenCount = HN - Parity;
			initialNode->insertAtLeaf(insertIndex, key, data);
		}
		else {
			int i;
			for (i = N - 1; i >= insertIndex; --i) {
				rightNode->keys[i - HN + 1] = MoveVal(initialNode->keys[i]);
				rightNode->ptrs[i - HN + 1] = initialNode->ptrs[i];
			}
			rightNode->keys[i - HN + 1] = MoveVal(key);
			rightNode->ptrs[i - HN + 1] = reinterpret_cast<Node*>(data);

			for (; i >= HN; --i) {
				rightNode->keys[i - HN] = MoveVal(initialNode->keys[i]);
				rightNode->ptrs[i - HN] = initialNode->ptrs[i];
			}
			rightNode->childrenCount = HN - Parity + 1;
			initialNode->childrenCount = HN;
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
			for (int i = N; i > HN; --i) {
				outNewNode->ptrs[i - HN] = initialNode->ptrs[i];
				outNewNode->keys[i - HN - 1] = MoveVal(initialNode->keys[i - 1]);
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
				outNewNode->keys[i - HN - 1] = MoveVal(initialNode->keys[i - 1]);
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
				outNewNode->keys[i - 1 - HN - 1] = MoveVal(initialNode->keys[i - 1]);
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

		return MoveVal(poppedKey);
	}
	
	// Returns key index that was deleted
	int deleteKeyAndPtr(const KeyType& key, Node* ptr) {
		// PERF: possible to save 20 ms here.
		// Can leave empty stuff and fix it during later stages (eg redistribution or even node delete)
		// Can avoid double search if passing more params can detect where the ptr is (before/at/after key)
		// Can perform binary search when deleting key
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

		// PERF: possible to move here
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
};

#endif // __NODE_H_