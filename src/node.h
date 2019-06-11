#ifndef __NODE_H_
#define __NODE_H_

#include <array>
#include <utility>
#include <cassert>
#include <string>
#include <iostream>
#include <functional>

typedef unsigned int uint;

// PERF: check performance vs list
template<typename ArrayType, std::size_t ArraySize>
void insertAtArray(std::array<ArrayType, ArraySize>& arr, uint lastIndex, uint location, const ArrayType& elem) {
	assert(lastIndex < ArraySize);
	assert(location <= lastIndex);
	for (uint i = lastIndex; i > location; --i) {
		arr[i] = std::move(arr[i - 1]);
	}
	arr[location] = elem;
}


template<typename KeyType, typename DataType, uint N>
struct Node {
	typedef std::pair<uint, bool> ElemIndex;

	bool isLeaf;
	uint childrenCount;
	Node* parent;
	// 2 seperate arrays for better cache management, since iterating keys only is frequent.
	std::array<KeyType, N> keys;
	std::array<Node*, N + 1> ptrs;


	uint uid;

	// this does not resolve to actual struct wide static 
	// but static for this specific template instantiation
	static const uint Parity = N % 2;
	static const uint HN = N / 2 + Parity;

	Node()
		: isLeaf(false) 
		, childrenCount(0)
		, parent(nullptr) {
		static uint total_uids = 0;
		uid = total_uids++; 
	}

	bool isRoot() const {
		return parent == nullptr;
	}

	DataType* getAsData(uint index) {
		assert(isLeaf);
		assert(index <= childrenCount);
		return reinterpret_cast<DataType*>(ptrs[index - 1]);
	}

	void setAsData(uint index, DataType* data) {
		assert(isLeaf);
		assert(index <= childrenCount);
		ptrs[index - 1] = reinterpret_cast<Node*>(data);
	}

	ElemIndex getIndexOf(const KeyType& key) {
		
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
			return ElemIndex(0, false);
		}

		uint left = 0;
		uint right = childrenCount - 1;
		uint middle = 0;

		while (left <= right) {
			middle = left + (right - left) / 2;
			
			if (key == keys[middle]) {
				return ElemIndex(middle + 1, true);
			}

			if (key > keys[middle]) {
				left = middle + 1;
			}
			else {
				right = middle - 1;
			}
		}

		// when not found we still want to return the location
		return ElemIndex(left, false);
	}

	void insertAtLeaf(uint index, const KeyType& key, DataType* data) {
		assert(getIndexOf(key).second == false); // This should not exist.
		assert(isRoot() || childrenCount >= N / 2);
		assert(isLeaf);

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount, index, reinterpret_cast<Node*>(data));
		childrenCount++;
	}

	void insertAtInternal(uint index, const KeyType& key, Node* node) {
		assert(getIndexOf(key).second == false); // This should not exist.
		assert(isRoot() || childrenCount + 1 >= N / 2);
		assert(!isLeaf);

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount + 1, index + 1, node);
		childrenCount++;
	}

	static Node* splitAndInsertLeaf(Node* initialNode, uint insertIndex, const KeyType& key, DataType* data) {
		assert(initialNode->childrenCount == N);
	
		Node* rightNode = new Node();
		rightNode->isLeaf = true;
		if (insertIndex < HN) {
			// Our element is in the left node

			// PERF: split loops for better cache
			for (int i = N - 1; i >= HN; --i) {
				rightNode->keys[i - HN] = std::move(initialNode->keys[i]);
				rightNode->ptrs[i - HN] = initialNode->ptrs[i];
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
	static KeyType splitAndInsertInternal(Node* initialNode, Node*& outNewNode, uint insertIndex, const KeyType& key, Node* ptrInsert) {
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

			outNewNode->childrenCount = HN;
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

			outNewNode->childrenCount = HN;
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

			outNewNode->childrenCount = HN - 1;
			initialNode->childrenCount = HN;

			outNewNode->insertAtInternal(insertIndex - HN - 1, key, ptrInsert);
		}
		assert(outNewNode->childrenCount >= N / 2);
		for (uint i = 0; i < outNewNode->childrenCount + 1; ++i) {
			outNewNode->ptrs[i]->parent = outNewNode;
		}

		return std::move(poppedKey);
	}
};

template<typename KeyType, typename DataType, uint N>
struct ExactLoc {
	typedef Node<KeyType, DataType, N> TNode;

	TNode* leaf;
	uint index;
	bool exists;

	ExactLoc() {}

	ExactLoc(TNode* leaf, std::pair<uint, bool> elemIndex)
		: leaf(leaf)
		, index(elemIndex.first)
		, exists(elemIndex.second) {}
};

// Requirements for types:
// key: operator< & operator==, movable, copy-constructuble
// 
template<typename KeyType, typename DataType, uint N = 10>
struct Tree {
	typedef Node<KeyType, DataType, N> TNode;
	typedef ExactLoc<KeyType, DataType, N> TExactLoc;

	TNode* root;
	
	uint height;
	uint leaves;
	uint nodes;

	uint elementCount;

	Tree() {
		root = new TNode();
		root->isLeaf = true;
		elementCount = 0;
		height = 0;
		nodes = 1;
		leaves = 1;
	}

	// Return if an insert was actually made.
	bool set(const KeyType& key, DataType* data) {
		return insertKeyVal(key, data);
	}

	bool maybe_add(const KeyType& key, DataType* data) {
		return insertKeyVal(key, data, false);
	}

	DataType* get(const KeyType& key) const {
		TExactLoc loc = findKey(key);
		if (!loc.exists) {
			return nullptr;
		}
		return loc.leaf->getAsData(loc.index);
	}

	// Return true if actually removed something
	bool remove(const KeyType& key) {
		return false;
	}

	uint size() const {
		return elementCount;
	}

	void clear() {

	}

	bool empty() const {
		return size() == 0;
	}

private:
	// actual implementations
	TExactLoc findKey(const KeyType& key) const {
		std::pair<uint, bool> nextLoc;
		TNode* nextNode = root;

		while (!nextNode->isLeaf) {
			 nextLoc = nextNode->getIndexOf(key);
			 nextNode = nextNode->ptrs[nextLoc.first];
		}
		nextLoc = nextNode->getIndexOf(key);
		return TExactLoc(nextNode, nextLoc);
	}

	// PERF: maybe const & for location? test first
	void setAtIt(TExactLoc location, DataType* data) {
		assert(location.exists);
		location.leaf->setAsData(location.index, data);
	}

	//// This updates the only case where insert has to overwrite values.
	//// that is when a new minKey is inserted.
	//TNode* updateMinKey(const KeyType& key) {
	//	TNode* nextNode = root;
	//	while (!nextNode->isLeaf) {
	//		nextNode->keys[0] = key;
	//		nextNode = nextNode->ptrs[0];
	//	}
	//	// do NOT update the leaf node! let insert code handle this.
	//	return nextNode;
	//}

	bool insertKeyVal(const KeyType& key, DataType* data, bool modifyIfExists = true) {
		TExactLoc location = findKey(key);
		if (location.exists) {
			if (modifyIfExists) {
				setAtIt(location, data);
			}
			return false;
		}

		if (location.leaf->childrenCount < N) {
			location.leaf->insertAtLeaf(location.index, key, data);
		}
		else {
			// split node,
			TNode* second = TNode::splitAndInsertLeaf(location.leaf, location.index, key, data);
			second->isLeaf = true;
			nodes++;

			// now update parent, maybe multiple parents
			insertInParent(location.leaf, second, second->keys[0]);
		}
		elementCount++;
		return true;
	}

	// use after split to update leftNode, new rightNode the tree parent
	void insertInParent(TNode* leftNode, TNode* rightNode, const KeyType& rightMinKey) {

		if (leftNode->isRoot()) {
			assert(leftNode->parent == nullptr);
			root = new TNode();
			nodes++;
			leftNode->parent = root;
			rightNode->parent = root;
			root->ptrs[0] = leftNode;
			root->ptrs[1] = rightNode;
			root->keys[0] = rightMinKey;
			root->childrenCount = 1;
			height++;
			return;
		}
		
		
		TNode* parent = leftNode->parent;

		// PERF: maybe cache something to avoid searching in the parent again? Path from root in first search down?
		std::pair<uint, bool> insertLoc = parent->getIndexOf(rightMinKey);
		assert(insertLoc.second == false);
		
		if (parent->childrenCount < N) {
			parent->insertAtInternal(insertLoc.first, rightMinKey, rightNode);
			rightNode->parent = parent;
		}
		else {
			TNode* added = nullptr;
			KeyType poppedKey = TNode::splitAndInsertInternal(parent, added, insertLoc.first, rightMinKey, rightNode);
			nodes++;
			added->isLeaf = false;
			added->parent = parent;
			insertInParent(parent, added, poppedKey);
		}
	}

public:

	void validate_ptrs() const {
		std::function<void(TNode*)> for_node;

		for_node = [&](TNode* node) -> void {
			if (node->isLeaf) {
				return;
			}

			if (node->childrenCount >= 1) {
				if (node->ptrs[0] == node->ptrs[1]) {
					dot_print_node(node);
					std::cerr << "found double ptr node!\n";
					getchar();
				}
			}

			for (uint i = 0; i < node->childrenCount + 1; ++i) {
				if (node->ptrs[i]->parent != node) {
					std::cerr << "found incorrect node!\n";
					dot_print_node(node);
					getchar();
				}
				for_node(node->ptrs[i]);
			}

		};

		for_node(root);
	}

	void dot_print() const {
		dot_print_node(root);
	}

	void dot_print_node(TNode* start) const {
		constexpr char nl = '\n';
		auto& out = std::cerr;

		out << "digraph tree {" << nl;
		out << "node [shape=record];" << nl;
		
		std::function<void(TNode*)> print_info;
		std::function<void(TNode*)> print_conn;

		print_info = [&](TNode* node) -> void {
			out << "node_id" << node->uid << " [shape=record, label=\"";
			if (!node->isLeaf) {
				out << "<f" << 0 << "> # |";
			}
			for (uint i = 0; i < N; ++i) {
				if (i < node->childrenCount) {
					if (!node->isLeaf) {
						out << "<f" << i+1 << "> " << node->keys[i] << "|";
					}
					else {
						out << "<f" << i+1 << "> " << node->keys[i] << "\\n" << *node->getAsData(i+1) << "|";
					}
					
				}
				else {
					out << "<f" << i+1 << "> ~|";
				}
			}
			out << '\b';
			out << "\"];" << nl;

			if (!node->isLeaf) {
				for (uint i = 0; i < node->childrenCount + 1; ++i) {
					print_info(node->ptrs[i]);
				}
			}
		};

		print_info(start);

		print_conn = [&](TNode* node) -> void {
			if (!node->isLeaf) {
				for (uint i = 0; i < node->childrenCount + 1; ++i) {
					out << "\"node_id" << node->uid << "\":f" << i << " -> ";
					out << "node_id" << node->ptrs[i]->uid << ";" << nl;
					print_conn(node->ptrs[i]);
				}
			}
			if (!node->isRoot()) {
				out << "\"node_id" << node->uid << "\" -> ";
				out << "node_id" << node->parent->uid << " [color=brown];" << nl;
			}
		};

		print_conn(start);
		out << "}" << nl;
	}
};



#endif // __NODE_H_