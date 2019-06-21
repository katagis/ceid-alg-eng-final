#ifndef __TREE_H_
#define __TREE_H_

#include "node.h"


// Requirements for types:
// key: operator< & operator==, movable, copy-constructuble
// 

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
		return leaf != nullptr;
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

// Requirements for types:
// key: operator< & operator==, movable, copy-constructuble
// 
template<typename KeyType, typename DataType, uint N = 10>
struct Tree {
	typedef Node<KeyType, DataType, N> TNode;
	typedef Iterator<KeyType, DataType, N> Iterator;

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
		Iterator loc = find(key);
		if (!loc.exists) {
			return false;
		}
		outData = loc.leaf->getAsData(loc.index);
		return true;
	}

	// Return true if actually removed something
	bool remove(const KeyType& key) {
		Iterator loc = find(key);
		if (!loc.exists) {
			return false;
		}
		deleteAt(loc);
		return true;
	}

	// Remove a key and return the pointer to the element if it existed.
	bool removePop(const KeyType& key, DataType*& popped) {
		Iterator loc = find(key);
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
		if (root->childrenCount == 0) {
			return;
		}
		for (int i = 0; i <= root->childrenCount; ++i) {
			clearNode(root->ptrs[i]);
		}
		init();
	}

	// Clear with a destructor for all elements
	template<typename PRED>
	void clearDestructor(PRED destructor) {
		for (Iterator it = first(); it.isValid(); ++it) {
			destructor(it.value());
		}
		clear();
	}

	bool empty() const {
		return size() == 0;
	}

	// get an iterator to the leftmost item in the tree
	Iterator first() const {
		if (empty()) {
			return Iterator(nullptr, 0, false);
		}
		TNode* nextNode = root;

		while (!nextNode->isLeaf) {
			nextNode = nextNode->ptrs[0];
			INCR_BLOCKS();
		}
		assert(nextNode->isLeaf);
		return Iterator(nextNode , 0, true);
	}

	Iterator find(const KeyType& key) const {
		int nextLoc;
		TNode* nextNode = root;

		while (!nextNode->isLeaf) {
			nextLoc = nextNode->getIndexOf(key);
			nextNode = nextNode->ptrs[nextLoc];
			INCR_BLOCKS();
		}

		bool found = false;
		nextLoc = nextNode->getIndexOfFound(key, found);

		return Iterator(nextNode, nextLoc - 1, found);
	}

private:
	void init() {
		root->isLeaf = true;
		root->childrenCount = 0;
		elementCount = 0;
		height = 0;
		nodes = 1;
	}

	void setAtIt(Iterator location, DataType* data) {
		assert(location.exists);
		location.leaf->setAsData(location.index, data);
	}


	DataType* deleteAt(Iterator location) {
		DataType* data = location.leaf->getAsData(location.index);
		deleteEntryLeaf(location.leaf, location.leaf->keys[location.index], location.leaf->ptrs[location.index]);
		elementCount--;
		return data;
	}

	void insertAt(Iterator location, const KeyType& key, DataType* data) {
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
		Iterator location = find(key);
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
			newRoot->parent = nullptr;
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

	void deleteEntryLeaf(TNode* initial, const KeyType& key, TNode* ptr) {
		assert(initial);
		assert(initial->isLeaf);

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

		int insertLoc = parent->getIndexOf(rightMinKey);

		if (parent->childrenCount < N) {
			parent->insertAtInternal(insertLoc, rightMinKey, rightNode);
			rightNode->parent = parent;
		}
		else {
			TNode* added = nullptr;
			KeyType poppedKey = TNode::splitAndInsertInternal(parent, added, insertLoc, rightMinKey, rightNode);
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
				TNode* nextLeaf = node->getNextLeaf();

				if (nextLeaf) {
					if (nextLeaf->keys[0] <= node->keys[node->childrenCount - 1]) {
						std::cerr << "found incorrect next node ptr!\n";
						getchar();
					}
				}

				return;
			}

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
						out << "<f" << i + 1 << "> " << node->keys[i] << "|";
					}
					else {
						out << "<f" << i + 1 << "> " << node->keys[i] << "\\n" << *node->getAsData(i + 1) << "|";
					}

				}
				else {
					out << "<f" << i + 1 << "> ~|";
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

#endif // __TREE_H_