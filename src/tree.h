#ifndef __TREE_H_
#define __TREE_H_

#include "node.h"

// Not an actuall stl like iterator but good enough for our example
// can be used to linearly iterate over elements with O(1) increment for the next element
template<typename KeyType, typename DataType, uint N>
struct Iterator {
	typedef Node<KeyType, DataType, N> TNode;

	// order is important here for best performance
	bool exists;
	TNode* leaf;
	int index;

	Iterator() {}

	Iterator(TNode* leaf, int elemIndex, bool found)
		: leaf(leaf)
		, index(elemIndex)
		, exists(found) {}


	void operator++() {
		next();
	}

	// Increments iterator to the next element. May skip to another leaf
	void next() {
		++index;
		if (index >= leaf->childrenCount) {
			nextLeaf();
			INCR_BLOCKS();
		}
	}

	// true if the current position has any element that can be accessed.
	// can be used for iteration loops as ending condition
	bool isValid() const {
		return leaf != NULL;
	}

	// Skip to the first element of the next leaf
	void nextLeaf() {
		index = 0;
		leaf = leaf->getNextLeaf();
	}

	const KeyType& key() const {
		return leaf->keys[index];
	}

	DataType* value() const {
		return leaf->getAsData(index);
	}

	DataType*& valueAsMutablePtr() {
		return leaf->getAsDataMutable(index);
	}
};

template<typename KeyType, typename DataType, uint N = 16>
struct Tree {
	typedef Node<KeyType, DataType, N> TNode;
	typedef Iterator<KeyType, DataType, N> TIterator;

	static const int Parity = N % 2;
	static const int HN = N / 2 + Parity;

	TNode* root;

	uint height;
	uint nodes;

	uint elementCount;

	Tree() {
		root = new TNode();
		init();
	}

	~Tree() {
		clear();
		delete root;
	}

	// Return if an insert was actually made.
	bool set(const KeyType& key, DataType* data) {
		return insertKeyVal(key, data);
	}

	bool maybe_add(const KeyType& key, DataType* data) {
		return insertKeyVal(key, data, false);
	}

	bool get(const KeyType& key, DataType*& outData) const {
		TIterator loc = find(key);
		if (!loc.exists) {
			return false;
		}
		outData = loc.leaf->getAsData(loc.index);
		return true;
	}

	// Return true if actually removed something
	bool remove(const KeyType& key) {
		TIterator loc = find(key);
		if (!loc.exists) {
			return false;
		}
		deleteAt(loc);
		return true;
	}

	// Remove a key and return the pointer to the element if it existed.
	bool removePop(const KeyType& key, DataType*& popped) {
		TIterator loc = find(key);
		if (!loc.exists) {
			return false;
		}
		popped = loc.value();
		deleteAt(loc);
		return true;
	}

	uint size() const {
		return elementCount;
	}

	void clearNode(TNode* node) {
		if (node->isLeaf) {
			delete node;
			return;
		}
		for (int i = 0; i <= node->childrenCount; ++i) {
			clearNode(node->ptrs[i]);
		}
		delete node;
	}

	void clear() {
		if (!root->isLeaf) {
			for (int i = 0; i <= root->childrenCount; ++i) {
				clearNode(root->ptrs[i]);
			}
		}
		init();
	}

	// Clear with a destructor for all elements
	template<typename PRED>
	void clearDestructor(PRED destructor) {
		for (TIterator it = first(); it.isValid(); ++it) {
			destructor(it.value());
		}
		clear();
	}

	bool empty() const {
		return size() == 0;
	}

	// get an iterator to the leftmost item in the tree
	TIterator first() const {
		if (empty()) {
			return TIterator(NULL, 0, false);
		}
		TNode* nextNode = root;

		while (!nextNode->isLeaf) {
			nextNode = nextNode->ptrs[0];
			INCR_BLOCKS();
		}
		assert(nextNode->isLeaf);
		return TIterator(nextNode , 0, true);
	}

	TIterator find(const KeyType& key) const {
		int nextLoc;
		TNode* nextNode = root;

		while (!nextNode->isLeaf) {
			nextLoc = nextNode->getIndexOf(key);
			nextNode = nextNode->ptrs[nextLoc];
			INCR_BLOCKS();
		}

		bool found = false;
		nextLoc = nextNode->getIndexOfFound(key, found);

		return TIterator(nextNode, nextLoc - 1, found);
	}

private:
	void init() {
		root->isLeaf = true;
		root->childrenCount = 0;
		elementCount = 0;
		height = 0;
		nodes = 1;
	}

	void setAtIt(TIterator location, DataType* data) {
		assert(location.exists);
		location.leaf->setAsData(location.index, data);
	}


	DataType* deleteAt(TIterator location) {
		DataType* data = location.leaf->getAsData(location.index);
		deleteEntryLeaf(location);
		elementCount--;
		return data;
	}

	void insertAt(TIterator location, const KeyType& key, DataType* data) {
		if (location.leaf->childrenCount < N) {
			location.leaf->insertAtLeaf(location.index + 1, key, data);
		}
		else {
			// split node,
			TNode* second = TNode::splitAndInsertLeaf(location.leaf, location.index + 1, key, data);
			second->isLeaf = true;
			nodes++;

			// now update parent, maybe multiple parents
			insertInParent(location.leaf, second, second->keys[0]);
		}
		elementCount++;
	}

	bool insertKeyVal(const KeyType& key, DataType* data, bool modifyIfExists = true) {
		TIterator location = find(key);
		if (location.exists) {
			if (modifyIfExists) {
				setAtIt(location, data);
			}
			return false;
		}
		insertAt(location, key, data);
		return true;
	}


	void redistributeBetweenLeaves(TNode* left, TNode* right, const KeyType& keyInBetween) {
		assert(left->keys[0] < right->keys[0]);
		assert(keyInBetween <= right->keys[0]);
		assert(left->isLeaf);


		bool smallerLeft = left->childrenCount < right->childrenCount;
		TNode* parent = left->parent;

		if (smallerLeft) {
			left->insertAtLeaf(left->childrenCount, right->keys[0], reinterpret_cast<DataType*>(right->ptrs[0]));

			bool found = false;
			int loc = parent->getIndexOfFound(keyInBetween, found);

			deleteFromArrayAt(right->ptrs, right->childrenCount, 0);
			deleteFromArrayAt(right->keys, right->childrenCount, 0);

			if (found) {
				parent->keys[loc - 1] = MoveVal(right->keys[0]);
			}
			right->childrenCount--;
		}
		else {
			right->insertAtLeaf(0, left->keys[left->childrenCount - 1], reinterpret_cast<DataType*>(left->ptrs[left->childrenCount - 1]));

			bool found = false;
			int loc = parent->getIndexOfFound(keyInBetween, found);

			if (found) {
				parent->keys[loc - 1] = MoveVal(left->keys[left->childrenCount - 1]);
			}
			left->childrenCount--;
		}
	}

	void redistributeBetween(TNode* left, TNode* right, const KeyType& keyInBetween) {
		assert(left->keys[0] < right->keys[0]);
		assert(keyInBetween <= right->keys[0]);
		assert(!left->isLeaf);

		bool smallerLeft = left->childrenCount < right->childrenCount;
		TNode* parent = left->parent;

		if (smallerLeft) {
			left->insertAtInternal(left->childrenCount, keyInBetween, right->ptrs[0]);
			left->ptrs[left->childrenCount]->parent = left;

			int loc = parent->getIndexOf(keyInBetween);

			parent->keys[loc - 1] = MoveVal(right->keys[0]);

			deleteFromArrayAt(right->ptrs, right->childrenCount + 1, 0);
			deleteFromArrayAt(right->keys, right->childrenCount, 0);
			right->childrenCount--;
		}
		else {
			insertAtArray(right->keys, right->childrenCount, 0, keyInBetween);
			insertAtArray(right->ptrs, right->childrenCount + 1, 0, left->ptrs[left->childrenCount]);
			right->childrenCount++;
			right->ptrs[0]->parent = right;

			int loc = parent->getIndexOf(keyInBetween);

			parent->keys[loc - 1] = MoveVal(left->keys[left->childrenCount - 1]);
			left->childrenCount--;
		}

	}

	void deleteEntryInternal(TNode* initial, const KeyType& key, TNode* ptr) {
		assert(!initial->isLeaf);

		KeyType oldKey = key;
		const int delPos = initial->deleteKeyAndPtr(key, ptr);

		if (initial->isRoot()) {
			if (initial->childrenCount > 0 || height == 0) {
				return;
			}
			TNode* newRoot = initial->ptrs[0];
			newRoot->parent = NULL;
			root = newRoot;
			delete initial;
			nodes--;
			height--;
			return;
		}

		int index = initial->parent->getIndexOf(initial->keys[0]);

		// The selected pointer is not cached so we need a new block for the node.
		INCR_BLOCKS();

		KeyType& mergeKey = (index == 0) ? initial->parent->keys[0] : initial->parent->keys[index - 1];
		
		TNode* left;
		TNode* right;
		
		if (index == 0) {
			// get the right 
			left = initial;
			right = initial->parent->ptrs[1];;
		}
		else {
			// get left
			left = initial->parent->ptrs[index - 1];
			right = initial;
		}

		bool CanMerge = left->childrenCount + 1
				+ right->childrenCount + 1 <= N + 1;

		if (CanMerge) {
			const int leftChildren = left->childrenCount;
			const int rightChildren = right->childrenCount;

			left->keys[leftChildren] = mergeKey;
			left->ptrs[leftChildren + 1] = right->ptrs[0];
			left->ptrs[leftChildren + 1]->parent = left;

			for (int i = 0; i < rightChildren; ++i) {
				left->keys[i + leftChildren + 1] = right->keys[i];
				left->ptrs[i + leftChildren + 1 + 1] = right->ptrs[i + 1];
				left->ptrs[i + leftChildren + 1 + 1]->parent = left;
			}
			left->childrenCount = leftChildren + rightChildren + 1;
			deleteEntryInternal(left->parent, mergeKey, right);
			delete right;
			nodes--;
		}
		else { // redistribute
			bool shouldRedistribute = std::abs(left->childrenCount - right->childrenCount) > 1;
			if (shouldRedistribute) {
				redistributeBetween(left, right, mergeKey);
			}
		}
	}

	void deleteEntryLeaf(TIterator location) {
		TNode* initial = location.leaf;
		assert(initial);
		assert(initial->isLeaf);
		assert(location.exists);

		KeyType oldKey = location.key();
		initial->deleteKeyValAt(location.index);

		if (initial->isRoot()) {
			if (initial->childrenCount > 0 || height == 0) {
				return;
			}
			TNode* newRoot = initial->ptrs[0];
			newRoot->parent = NULL;
			root = newRoot;
			delete initial;
			nodes--;
			height--;
			return;
		}

		if (initial->childrenCount >= HN) {
			return;
		}
		
		INCR_BLOCKS();
		int index = initial->parent->getIndexOf(initial->keys[0]);

		TNode* left;
		TNode* right;
		KeyType& mergeKey = (index == 0) ? initial->parent->keys[0] : initial->parent->keys[index - 1];
		
		if (index == 0) {
			right = initial->parent->ptrs[1];
			left = initial;
		}
		else {
			right = initial;
			left = initial->parent->ptrs[index - 1];
		}

		int totalChildren = left->childrenCount + right->childrenCount;
		
		if (totalChildren <= N) {
			for (int i = totalChildren - 1; i >= left->childrenCount; --i) {
				left->ptrs[i] = right->ptrs[i - left->childrenCount];
				left->keys[i] = right->keys[i - left->childrenCount];
			}
			left->childrenCount = totalChildren;
			left->setNextLeaf(right->getNextLeaf());
			deleteEntryInternal(right->parent, mergeKey, right);
			delete right;
			nodes--;
		}
		else { // redistribute
			redistributeBetweenLeaves(left, right, mergeKey);
		}
	}

	// use after split to update leftNode, new rightNode the tree parent
	void insertInParent(TNode* leftNode, TNode* rightNode, const KeyType& rightMinKey) {

		if (leftNode->isRoot()) {
			assert(leftNode->parent == NULL);
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

		int insertLoc = parent->getIndexOf(rightMinKey);

		if (parent->childrenCount < N) {
			parent->insertAtInternal(insertLoc, rightMinKey, rightNode);
			rightNode->parent = parent;
		}
		else {
			TNode* added = NULL;
			KeyType poppedKey = TNode::splitAndInsertInternal(parent, added, insertLoc, rightMinKey, rightNode);
			nodes++;
			added->isLeaf = false;
			added->parent = parent;
			insertInParent(parent, added, poppedKey);
		}
	}

public:
	// checks and verfies the internal node connections & other 'assertations'
	// this was used only during development as a debugging utility.
	void checkIntegrity() {
		checkNode(root);
	}

	// To be able to use this KeyType AND DataType must have an operator<< for printing
	void dotPrint() {
		printAsTree(std::cout, root);
	}

private:
	void checkNode(TNode* node) {
		if (node->childrenCount < 0 || node->childrenCount > N) {
			std::cerr << "found incorrect children count!\n";
			abort();
		}

		if (node->isLeaf) {
			TNode* nextLeaf = node->getNextLeaf();

			if (nextLeaf) {
				if (nextLeaf->keys[0] <= node->keys[node->childrenCount - 1]) {
					std::cerr << "found incorrect next node ptr!\n";
					abort();
				}
			}

			return;
		}

		if (node->childrenCount >= 1) {
			if (node->ptrs[0] == node->ptrs[1]) {
				std::cerr << "found double ptr node!\n";
				printAsTree(std::cerr, node);
				abort();
			}
		}

		if (node->childrenCount >= 2) {
			if (node->keys[0] > node->keys[1]) {
				std::cerr << "found key error!\n";
				abort();
			}
		}

		for (int i = 0; i < node->childrenCount + 1; ++i) {
			if (node->ptrs[i]->parent != node) {
				std::cerr << "found incorrect node!\n";
				printAsTree(std::cerr, node);
				abort();
			}
			checkNode(node->ptrs[i]);
		}
	}

	static void printNodeInfo(std::ostream& out, TNode* node) {
		out << "node_id" << node->uid << " [shape=record, label=\"";
		if (!node->isLeaf) {
			out << "<f" << 0 << "> # |";
		}
		for (int i = 0; i < N; ++i) {
			if (i < node->childrenCount) {
				if (!node->isLeaf) {
					out << "<f" << i + 1 << "> " << node->keys[i] << "|";
				}
				else {
					out << "<f" << i + 1 << "> " << node->keys[i] << "\\n" << *node->getAsData(i) << "|";
				}

			}
			else {
				out << "<f" << i + 1 << "> ~|";
			}
		}
		out << '\b';
		out << "\"];\n";

		if (!node->isLeaf) {
			for (int i = 0; i < node->childrenCount + 1; ++i) {
				printNodeInfo(out, node->ptrs[i]);
			}
		}
	}

	static void printConnection(std::ostream& out, TNode* node) {

		// Print children connections
		if (!node->isLeaf) {
			for (int i = 0; i < node->childrenCount + 1; ++i) {
				out << "\"node_id" << node->uid << "\":f" << i << " -> ";
				out << "node_id" << node->ptrs[i]->uid << ";\n";
				printConnection(out, node->ptrs[i]);
			}
		}
		// Print parent pointers for debugging
		//if (!node->isRoot()) {
		//	out << "\"node_id" << node->uid << "\" -> ";
		//	out << "node_id" << node->parent->uid << " [color=brown];\n";
		//}
		
		// Print next ptr connection
		// Using 'rank' messes up some dot generators but it is required if printing next ptrs.
		//if (node->isLeaf && node->next != NULL) {
		//	out << "\"node_id" << node->uid << "\" -> ";
		//	out << "node_id" << node->next->uid << " [color=brown];\n";
		//	out << "{rank = same; node_id" << node->uid << "; node_id" << node->next->uid << "};\n";
		//}
	}

	static void printAsTree(std::ostream& out, TNode* start) {
		out << "digraph tree {\n";
		out << "node [shape=record];\n";

		printNodeInfo(out, start);
		printConnection(out, start);

		out << "}\n";
	}
};

#endif // __TREE_H_