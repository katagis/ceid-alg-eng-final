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

	std::pair<int, bool> foundIndex = { 0, false };
	for (int i = 0; i < arrSize; ++i) {
		if (arr[i] == elem) {
			foundIndex = { i, true };
			break;
		}
	}
	assert(foundIndex.second);

	deleteFromArrayAt(arr, arrSize, foundIndex.first);
	return foundIndex.first;
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

		int left = 0;
		int right = childrenCount - 1;
		int middle = 0;

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

	void replaceKey(const KeyType& from, const KeyType& to) {
		std::pair<uint, bool> loc = getIndexOf(from);
		assert(loc.second);
		keys[loc.first] = to;
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

template<typename KeyType, typename DataType, uint N>
struct ExactLoc {
	typedef Node<KeyType, DataType, N> TNode;

	TNode* leaf;
	int index;
	bool exists;

	ExactLoc() {}

	ExactLoc(TNode* leaf, std::pair<int, bool> elemIndex)
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
	
	static const int Parity = N % 2;
	static const int HN = N / 2 + Parity;

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
		return removePop(key) != nullptr;
	}

	// Remove a key and return the pointer to the element if it existed.
	DataType* removePop(const KeyType& key) {
		TExactLoc loc = findKey(key);
		if (!loc.exists) {
			return nullptr;
		}
		return deleteAt(loc);
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
		std::pair<int, bool> nextLoc;
		TNode* nextNode = root;

		while (!nextNode->isLeaf) {
			 nextLoc = nextNode->getIndexOf(key);
			 nextNode = nextNode->ptrs[nextLoc.first];
		}
		nextLoc = nextNode->getIndexOf(key);
		return TExactLoc(nextNode, nextLoc);
	}

	// PERF: maybe pass location by const&, do tests
	void setAtIt(TExactLoc location, DataType* data) {
		assert(location.exists);
		location.leaf->setAsData(location.index, data);
	}

	
	void redistributeBetween(TNode* left, TNode* right, bool smallerLeft, const KeyType& keyInBetween) {
		assert(left->keys[0] < right->keys[0]);
		assert(keyInBetween <= right->keys[0]);
		

		if (smallerLeft) {
			assert(left->childrenCount < right->childrenCount);
		}
		else {
			assert(right->childrenCount < left->childrenCount);
		}

		TNode* parent = left->parent;
		if (!left->isLeaf) {
			if (smallerLeft) {
				left->insertAtInternal(left->childrenCount, keyInBetween, right->ptrs[0]);
				left->ptrs[left->childrenCount]->parent = left;

				std::pair<int, bool> loc = parent->getIndexOf(keyInBetween);
				assert(loc.second);
				parent->keys[loc.first - 1] = std::move(right->keys[0]);

				deleteFromArrayAt(right->ptrs, right->childrenCount + 1, 0);
				deleteFromArrayAt(right->keys, right->childrenCount, 0);
				right->childrenCount--;
			}
			else {
				insertAtArray(right->keys, right->childrenCount, 0, keyInBetween);
				insertAtArray(right->ptrs, right->childrenCount + 1, 0, left->ptrs[left->childrenCount]);
				right->childrenCount++;
				right->ptrs[0]->parent = right;

				std::pair<int, bool> loc = parent->getIndexOf(keyInBetween);
				assert(loc.second);

				parent->keys[loc.first - 1] = std::move(left->keys[left->childrenCount - 1]);

				left->childrenCount--;
			}
		}
		else {
			if (smallerLeft) {
				left->insertAtLeaf(left->childrenCount, right->keys[0], reinterpret_cast<DataType*>(right->ptrs[0]));

				std::pair<int, bool> loc = parent->getIndexOf(keyInBetween);
				
				deleteFromArrayAt(right->ptrs, right->childrenCount, 0);
				deleteFromArrayAt(right->keys, right->childrenCount, 0);


				if (loc.second) {
					parent->keys[loc.first - 1] = std::move(right->keys[0]);
				}

				
				right->childrenCount--;
			}
			else {
				right->insertAtLeaf(0, left->keys[left->childrenCount - 1], reinterpret_cast<DataType*>(left->ptrs[left->childrenCount - 1]));

				std::pair<int, bool> loc = parent->getIndexOf(keyInBetween);
				if (loc.second) {
					parent->keys[loc.first - 1] = std::move(left->keys[left->childrenCount - 1]);
				}

				left->childrenCount--;
			}
		}
	}

	/*void updateKey(const KeyType& oldKey, const KeyType& newKey) {
		if (root->isLeaf) {
			return;
		}
		TNode* iter = root;
		std::pair<uint, bool> keyLoc = iter->getIndexOf(oldKey);

		while (!keyLoc.second) {
			iter = iter->ptrs[keyLoc.first];
			if (iter->isLeaf) {
				//std::cerr << "Could not udpate key: " << oldKey << " to: " << newKey << std::endl;
				return;
			}
			keyLoc = iter->getIndexOf(oldKey);
		}
		iter->keys[keyLoc.first - 1] = newKey;
	}*/

	void deleteEntry(TNode* initial, const KeyType& key, TNode* ptr) {
		assert(initial);
		KeyType oldKey = key;
		const int delPos = initial->deleteKeyAndPtr(key, ptr);

		if (initial->isRoot()) {
			if (initial->childrenCount > 0 || height == 0) {
				return;
			}
			TNode* newRoot = initial->ptrs[0];
			newRoot->parent = nullptr;
			root = newRoot;
			delete initial;
			nodes--;
			height--;
			return;
		}

		if (initial->isLeaf) {
			if (initial->childrenCount >= HN) {
				if (delPos == 0) {
					//updateKey(oldKey, initial->keys[0]);
				}
				return;
			}
		}

		int index = initial->parent->getIndexOf(initial->keys[0]).first;
		
		TNode* merge;
		bool mergeToLeft = true;
		KeyType mergeKey;
		if (index == 0) {
			// get the right 
			merge = initial->parent->ptrs[1];
			mergeToLeft = false;
			mergeKey = initial->parent->keys[0];
		}
		else {
			// get left
			merge = initial->parent->ptrs[index - 1];
			mergeKey = initial->parent->keys[index - 1];
		}



		bool CanMerge = initial->childrenCount + merge->childrenCount <= N;

		if (!initial->isLeaf) {
			CanMerge = initial->childrenCount + 1 
				       + merge->childrenCount + 1 <= N + 1;
		}

		if (CanMerge) {
			int totalChildren = initial->childrenCount + merge->childrenCount;
			// can fit in a sigle node


			if (!initial->isLeaf) {
				if (!mergeToLeft) {
					const int leftChildren = initial->childrenCount;
					const int rightChildren = merge->childrenCount;
					
					merge->moveInfoInplaceInternal(0, rightChildren, leftChildren + 1, mergeKey);

					merge->ptrs[0] = initial->ptrs[0];
					merge->ptrs[0]->parent = merge;

					for (int i = 0; i < leftChildren; ++i) {
						merge->keys[i] = initial->keys[i];
						merge->ptrs[i + 1] = initial->ptrs[i + 1];
						merge->ptrs[i + 1]->parent = merge;
					}
					
					merge->childrenCount = leftChildren + rightChildren + 1;
				}
				else {
					const int leftChildren = merge->childrenCount;
					const int rightChildren = initial->childrenCount;

					merge->keys[leftChildren] = mergeKey;
					merge->ptrs[leftChildren + 1] = initial->ptrs[0];
					merge->ptrs[leftChildren + 1]->parent = merge;

					for (int i = 0; i < rightChildren; ++i) {
						merge->keys[i + leftChildren + 1 ] = initial->keys[i];
						merge->ptrs[i + leftChildren + 1 + 1] = initial->ptrs[i + 1];
						merge->ptrs[i + leftChildren + 1 + 1]->parent = merge;
					}
					merge->childrenCount = leftChildren + rightChildren + 1;
				}
			}
			else {
				if (!mergeToLeft) {
					merge->moveInfoInplaceLeaf(0, merge->childrenCount - 1, initial->childrenCount);

					for (int i = 0; i < initial->childrenCount; ++i) {
						merge->ptrs[i] = initial->ptrs[i];
						merge->keys[i] = initial->keys[i];
					}
					merge->childrenCount = totalChildren;
				}
				else {
					for (int i = totalChildren - 1; i >= merge->childrenCount; --i) {
						merge->ptrs[i] = initial->ptrs[i - merge->childrenCount];
						merge->keys[i] = initial->keys[i - merge->childrenCount];
					}
					merge->childrenCount = totalChildren;
				}
			}
			deleteEntry(initial->parent, mergeKey, initial);
			//if (!merge->isRoot()) {
			//	updateKey(oldKey, merge->keys[0]);
			//}
			delete initial;
			nodes--;
		}
		else { // redistribute
			if (initial->isLeaf) {
				if (mergeToLeft) {
					redistributeBetween(merge, initial, false, mergeKey);
				}
				else {
					redistributeBetween(initial, merge, true, mergeKey);
				}
				//updateKey(oldKey, initial->keys[0]);
			}
			else {
				bool shouldRedistribute = std::abs(initial->childrenCount - merge->childrenCount) > 1;

				if (shouldRedistribute) {
					if (mergeToLeft) {
						redistributeBetween(merge, initial, false, mergeKey);
					}
					else {
						redistributeBetween(initial, merge, true, mergeKey);
					}
					//updateKey(oldKey, initial->keys[0]);
				}
			}
		}
	}

	DataType* deleteAt(TExactLoc location) {
		DataType* data = location.leaf->getAsData(location.index);
		deleteEntry(location.leaf, location.leaf->keys[location.index - 1], location.leaf->ptrs[location.index - 1]);
		elementCount--;
		return data;
	}

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
		std::pair<int, bool> insertLoc = parent->getIndexOf(rightMinKey);
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
			if (node->childrenCount < 0 || node->childrenCount > N) {
				std::cerr << "found incorrect children count!\n";
				getchar();
			}

			if (node->isLeaf) {
				return;
			}


			/*for (int i = 0; i < node->childrenCount; ++i) {
				if (!this->get(node->keys[i])) {
					std::cerr << "found incorrect node reference :" << node->keys[i] << std::endl;
					dot_print_node(node);
					dot_print();
					getchar();
				}
			}*/

			if (node->childrenCount >= 1) {
				if (node->ptrs[0] == node->ptrs[1]) {
					dot_print_node(node);
					std::cerr << "found double ptr node!\n";
					getchar();
				}
			}

			if (node->childrenCount >= 2) {
				if (node->keys[0] > node->keys[1]) {
					this->dot_print();
					std::cerr << "found key error!\n";
					getchar();
				}
			}

			for (int i = 0; i < node->childrenCount + 1; ++i) {
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

	static void dot_print_node(TNode* start) {
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
			for (int i = 0; i < N; ++i) {
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
				for (int i = 0; i < node->childrenCount + 1; ++i) {
					print_info(node->ptrs[i]);
				}
			}
		};

		print_info(start);

		print_conn = [&](TNode* node) -> void {
			if (!node->isLeaf) {
				for (int i = 0; i < node->childrenCount + 1; ++i) {
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