#ifndef __TREE_H_
#define __TREE_H_

#include "node.h"


// Requirements for types:
// key: operator< & operator==, movable, copy-constructuble
// 


template<typename KeyType, typename DataType, uint N>
struct Iterator {
	typedef Node<KeyType, DataType, N> TNode;

	TNode* leaf;
	int index;
	bool exists;

	Iterator() {}

	Iterator(TNode* leaf, int elemIndex, bool found)
		: leaf(leaf)
		, index(elemIndex)
		, exists(found) {}
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
		Iterator loc = findKey(key);
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
		Iterator loc = findKey(key);
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
	Iterator findKey(const KeyType& key) const {
		int nextLoc;
		TNode* nextNode = root;
		int curHeight = height;

		while (curHeight--) {
			nextLoc = nextNode->getIndexOf(key);
			nextNode = nextNode->ptrs[nextLoc];
		}
		bool found = false;
		nextLoc = nextNode->getIndexOfFound(key, found);
		return Iterator(nextNode, nextLoc, found);
	}

	// PERF: maybe pass location by const&, do tests
	void setAtIt(Iterator location, DataType* data) {
		assert(location.exists);
		location.leaf->setAsData(location.index, data);
	}


	void redistributeBetweenLeaves(TNode* left, TNode* right, bool smallerLeft, const KeyType& keyInBetween) {
		assert(left->keys[0] < right->keys[0]);
		assert(keyInBetween <= right->keys[0]);
		assert(left->isLeaf);

		TNode* parent = left->parent;

		if (smallerLeft) {
			left->insertAtLeaf(left->childrenCount, right->keys[0], reinterpret_cast<DataType*>(right->ptrs[0]));

			bool found = false;
			int loc = parent->getIndexOfFound(keyInBetween, found);

			deleteFromArrayAt(right->ptrs, right->childrenCount, 0);
			deleteFromArrayAt(right->keys, right->childrenCount, 0);

			if (found) {
				parent->keys[loc - 1] = std::move(right->keys[0]);
			}
			right->childrenCount--;
		}
		else {
			right->insertAtLeaf(0, left->keys[left->childrenCount - 1], reinterpret_cast<DataType*>(left->ptrs[left->childrenCount - 1]));

			bool found = false;
			int loc = parent->getIndexOfFound(keyInBetween, found);

			if (found) {
				parent->keys[loc - 1] = std::move(left->keys[left->childrenCount - 1]);
			}
			left->childrenCount--;
		}
	}

	void redistributeBetween(TNode* left, TNode* right, bool smallerLeft, const KeyType& keyInBetween) {
		assert(left->keys[0] < right->keys[0]);
		assert(keyInBetween <= right->keys[0]);
		assert(!left->isLeaf);

		TNode* parent = left->parent;
		if (smallerLeft) {
			left->insertAtInternal(left->childrenCount, keyInBetween, right->ptrs[0]);
			left->ptrs[left->childrenCount]->parent = left;

			int loc = parent->getIndexOf(keyInBetween);

			parent->keys[loc - 1] = std::move(right->keys[0]);

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

			parent->keys[loc - 1] = std::move(left->keys[left->childrenCount - 1]);
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

		bool CanMerge = initial->childrenCount + 1
				+ merge->childrenCount + 1 <= N + 1;

		if (CanMerge) {
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
					merge->keys[i + leftChildren + 1] = initial->keys[i];
					merge->ptrs[i + leftChildren + 1 + 1] = initial->ptrs[i + 1];
					merge->ptrs[i + leftChildren + 1 + 1]->parent = merge;
				}
				merge->childrenCount = leftChildren + rightChildren + 1;
			}
			deleteEntryInternal(initial->parent, mergeKey, initial);
			delete initial;
			nodes--;
		}
		else { // redistribute
			bool shouldRedistribute = std::abs(initial->childrenCount - merge->childrenCount) > 1;
			if (shouldRedistribute) {
				if (mergeToLeft) {
					redistributeBetween(merge, initial, merge->childrenCount < initial->childrenCount, mergeKey);
				}
				else {
					redistributeBetween(initial, merge, initial->childrenCount < merge->childrenCount, mergeKey);
				}
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

		int index = initial->parent->getIndexOf(initial->keys[0]);

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

		if (CanMerge) {
			int totalChildren = initial->childrenCount + merge->childrenCount;
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
			deleteEntryInternal(initial->parent, mergeKey, initial);
			delete initial;
			nodes--;
		}
		else { // redistribute
			if (mergeToLeft) {
				redistributeBetweenLeaves(merge, initial, false, mergeKey);
			}
			else {
				redistributeBetweenLeaves(initial, merge, true, mergeKey);
			}
		}
	}

	DataType* deleteAt(Iterator location) {
		DataType* data = location.leaf->getAsData(location.index);
		deleteEntryLeaf(location.leaf, location.leaf->keys[location.index - 1], location.leaf->ptrs[location.index - 1]);
		elementCount--;
		return data;
	}

	bool insertKeyVal(const KeyType& key, DataType* data, bool modifyIfExists = true) {
		Iterator location = findKey(key);
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