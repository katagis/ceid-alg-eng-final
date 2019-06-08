#ifndef __NODE_H_
#define __NODE_H_

#include <array>
#include <utility>
#include <cassert>
#include <string>
#include <iostream>
#include <functional>

typedef unsigned int uint;

template<typename KeyType, typename DataType, uint N>
struct Node {
	typedef std::pair<uint, bool> ElemIndex;

	bool isLeaf;
	uint childrenCount;
	Node* parent;
	// 2 seperate arrays for better cache management, since iterating keys only is frequent.
	std::array<KeyType, N> keys;
	std::array<Node*, N> ptrs;


	uint uid;

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
		assert(index < childrenCount);
		return reinterpret_cast<DataType*>(ptrs[index]);
	}

	void setAsData(uint index, DataType* data) {
		assert(isLeaf);
		assert(index < childrenCount);
		ptrs[index] = reinterpret_cast<Node*>(data);
	}

	ElemIndex getIndexOf(const KeyType& key) {
		if (childrenCount == 0) {
			return ElemIndex(0, false);
		}
		
		// binary search here, with range from 0 -> childrenCount;

		uint left = 0;
		uint right = childrenCount - 1;
		uint middle = 0;

		while (left <= right) {
			middle = left + (right - left) / 2;
			
			if (key == keys[middle]) {
				return ElemIndex(middle, true);
			}

			if (key > keys[middle]) {
				left = middle + 1;
			}
			else {
				if (middle == 0) {
					return ElemIndex(0, false);
				}
				right = middle - 1;
			}
		}

		// when not found we still want to return the location
		return ElemIndex(left, false);
	}

private:
	// PERF: optimize array extension if it actually stays in the final
	template<typename ArrayType>
	void insertAtArray(std::array<ArrayType, N>& arr, uint lastIndex, uint location, const ArrayType& elem) {
		assert(lastIndex < N);
		assert(location <= lastIndex);
		for (uint i = lastIndex; i > location; --i) {
			arr[i] = std::move(arr[i - 1]);
		}
		arr[location] = elem;
	}

public:
	template<typename Ptr>
	void insertAt(uint index, const KeyType& key, Ptr* data) {
		assert(getIndexOf(key).second == false); // This should not exist.
		assert(isRoot() || childrenCount >= N / 2);

		// Only allow DataType or Node as Ptr, otherwise you used this function wrong.
		static_assert(std::is_same<Ptr, Node>::value || std::is_same<Ptr, DataType>::value, "Incorrect type at Node::insertAt");

		insertAtArray(keys, childrenCount, index, key);
		insertAtArray(ptrs, childrenCount, index, reinterpret_cast<Node*>(data));
		childrenCount++;
	}

	template<typename Ptr>
	static Node* splitAndInsert(Node* initialNode, uint insertIndex, const KeyType& key, Ptr* data) {
		assert(initialNode->childrenCount == N);
		static_assert(std::is_same<Ptr, Node>::value || std::is_same<Ptr, DataType>::value, "Incorrect type at Node::insertAt");
	
		Node* rightNode = new Node();
		
		if (insertIndex < N / 2) {
			// Our element is in the left node

			// PERF: split loops for better cache
			for (int i = N - 1; i >= N / 2; --i) {
				rightNode->keys[i - N/2] = std::move(initialNode->keys[i]);
				rightNode->ptrs[i - N/2] = initialNode->ptrs[i];
			}
			rightNode->childrenCount = N / 2;
			initialNode->childrenCount = N / 2;
			initialNode->insertAt(insertIndex, key, data);

			if (std::is_same<Ptr, Node>::value) {
				reinterpret_cast<Node*>(data)->parent = initialNode;
			}
		}
		else {
			// PERF: for now we do the same as above,
			// this moves some items 2 times and can be optimised
			
			for (int i = N - 1; i >= N / 2; --i) {
				rightNode->keys[i - N/2] = std::move(initialNode->keys[i]);
				rightNode->ptrs[i - N/2] = initialNode->ptrs[i];
			}
			rightNode->childrenCount = N / 2;
			initialNode->childrenCount = N / 2;
			rightNode->insertAt(insertIndex - N / 2, key, data);
		}

		if (!initialNode->isLeaf) {
			for (uint i = 0; i < rightNode->childrenCount; ++i) {
				rightNode->ptrs[i]->parent = rightNode;
			}
		}


		return rightNode;
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

		// This is a special case where search is not required,
		// because our root->keys[0] is the min key in our data.
		if (key < root->keys[0]) {
			return TExactLoc(root, std::pair<uint, bool>(0, false));
		}

		while (!nextNode->isLeaf) {
			 nextLoc = nextNode->getIndexOf(key);
			 const uint accessLoc = nextLoc.second || nextLoc.first == 0 ? nextLoc.first : nextLoc.first - 1;
			 assert(accessLoc >= 0 && accessLoc < nextNode->childrenCount && nextNode->childrenCount > 0);
			 nextNode = nextNode->ptrs[accessLoc];
		}
		nextLoc = nextNode->getIndexOf(key);
		return TExactLoc(nextNode, nextLoc);
	}

	// PERF: maybe const & for location? test first
	void setAtIt(TExactLoc location, DataType* data) {
		assert(location.exists);
		location.leaf->setAsData(location.index, data);
	}

	// This updates the only case where insert has to overwrite values.
	// that is when a new minKey is inserted.
	TNode* updateMinKey(const KeyType& key) {
		TNode* nextNode = root;
		while (!nextNode->isLeaf) {
			nextNode->keys[0] = key;
			nextNode = nextNode->ptrs[0];
		}
		// do NOT update the leaf node! let insert code handle this.
		return nextNode;
	}

	bool insertKeyVal(const KeyType& key, DataType* data, bool modifyIfExists = true) {
		TExactLoc location = findKey(key);
		if (location.exists) {
			if (modifyIfExists) {
				setAtIt(location, data);
			}
			return false;
		}

		// update the special case, a new minkey inserted.
		// dont forget to update the actual leaf
		if (!location.leaf->isLeaf) {
			location.leaf = updateMinKey(key); 
		}

		if (location.leaf->childrenCount < N) {
			location.leaf->insertAt(location.index, key, data);
		}
		else {
			// split node,
			TNode* second = TNode::splitAndInsert(location.leaf, location.index, key, data);
			second->isLeaf = true;
			nodes++;

			// now update parent, maybe multiple parents
			insertInParent(location.leaf, second);
		}
		elementCount++;
		return true;
	}

	// use after split to update leftNode, new rightNode the tree parent
	void insertInParent(TNode* leftNode, TNode* rightNode) {
		const KeyType& rightKey = rightNode->keys[0];
		if (leftNode->isRoot()) {
			assert(leftNode->parent == nullptr);
			root = new TNode();
			nodes++;
			leftNode->parent = root;
			rightNode->parent = root;
			root->insertAt(0, leftNode->keys[0], leftNode);
			root->insertAt(1, rightNode->keys[0], rightNode);
			height++;
			return;
		}
		
		
		TNode* parent = leftNode->parent;

		// PERF: maybe cache something to avoid searching in the parent again? Path from root in first search down?
		std::pair<uint, bool> insertLoc = parent->getIndexOf(rightNode->keys[0]);
		assert(insertLoc.second == false);
		
		if (parent->childrenCount < N) {
			parent->insertAt(insertLoc.first, rightNode->keys[0], rightNode);
			rightNode->parent = parent;
		}
		else {
			TNode* added = TNode::splitAndInsert(parent, insertLoc.first, rightNode->keys[0], rightNode);
			nodes++;
			added->isLeaf = false;
			added->parent = parent;
			insertInParent(parent, added);
		}
	}

public:
	void dot_print() const {
		constexpr char nl = '\n';
		auto& out = std::cerr;

		out << "digraph tree {" << nl;
		out << "node [shape=record];" << nl;
		
		std::function<void(TNode*)> print_info;
		std::function<void(TNode*)> print_conn;

		print_info = [&](TNode* node) -> void {
			out << "node_id" << node->uid << " [shape=record, label=\"";
			for (uint i = 0; i < N; ++i) {
				if (i < node->childrenCount) {
					if (!node->isLeaf) {
						out << "<f" << i << "> " << node->keys[i] << "|";
					}
					else {
						out << "<f" << i << "> " << node->keys[i] << "\\n" << *reinterpret_cast<DataType*>(node->ptrs[i]) << "|";
					}
					
				}
				else {
					out << "<f" << i << "> ~|";
				}
			}
			out << '\b';
			out << "\"];" << nl;

			if (!node->isLeaf) {
				for (uint i = 0; i < node->childrenCount; ++i) {
					print_info(node->ptrs[i]);
				}
			}
		};

		print_info(root);

		print_conn = [&](TNode* node) -> void {
			if (!node->isLeaf) {
				for (uint i = 0; i < node->childrenCount; ++i) {
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

		print_conn(root);
		out << "}" << nl;
	}
};



#endif // __NODE_H_