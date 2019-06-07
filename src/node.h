#ifndef __NODE_H_
#define __NODE_H_

#include <array>
#include <utility>
#include <cassert>
#include <string>

typedef unsigned int uint;

template<typename KeyType, typename DataType, uint A>
struct Node {
	typedef std::pair<KeyType, Node*> Elem;
	typedef std::pair<uint, bool> ElemIndex;

	bool isLeaf;
	uint childrenCount;
	std::array<Elem, A> child;

	Node()
		: isLeaf(false) 
		, childrenCount(0) {}

	DataType* getAsData(uint index) {
		assert(isLeaf);
		assert(index < childrenCount);
		return reinterpret_cast<DataType*>(child[index].second);
	}

	ElemIndex getIndexOf(const KeyType& key) {
		// binary search here, with range from 0 -> childrenCount;

		uint left = 0;
		uint right = childrenCount;
		uint middle;

		while (left <= right) {
			middle = left + (right - 1) / 2;
			
			if (key == child[m].first) {
				return ElemIndex(middle, true);
			}

			if (key < child[m].first) {
				left = middle - 1;
			}
			else {
				right = middle - 1;
			}
		}

		// when not found we still want to return the location
		return ElemIndex(middle, false);
	}
};

template<typename KeyType, typename DataType, uint A>
struct ExactLoc {
	Node* leaf;
	uint offset;
	bool exists;

	ExactLoc() {}

	ExactLoc(Node* leaf, const Node::ElemIndex& elemIndex)
		: leaf(leaf)
		, offset(elemIndex.first)
		, exists(elemIndex.second) {}
};

// required operators for key type: operator< & operator==
template<typename KeyType, typename DataType, uint A = 10>
struct Tree {
	Node root;
	
	uint height;
	uint leaves;
	
	uint elementCount;

	// Return if an insert was actually made.
	bool set(const KeyType& key, const DataType& data) {
		return false;
	}

	DataType* get(const KeyType& key) const {
		return nullptr;
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
	ExactLoc findKey(const KeyType& key) const {
		uint curHeight = 0;

		while (curHeight < height) {
			Node::ElemIndex nextLoc = root.getIndexOf(key);

		}
	}
};






#endif // __NODE_H_