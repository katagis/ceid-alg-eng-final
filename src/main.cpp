#ifndef _TESTS

#define DECLARE_EXTERN_TESTBENCH_VARS
#define COUNT_BLOCKS 1
#include "testbench.h"

#include "benchmark.h"
#include "tree.h"
#include "random_gen.h"
#include <LEDA/core/impl/ab_tree.h>
#include <LEDA/core/dictionary.h>
#include <iostream>

AggregateTimer timer;
BenchmarkResults bench;

void interactiveMode();

int main() {
	bench.SetTextOutput(true);
	interactiveMode();
	return 0;
}

int readOption() {
	int option = 0;
	do {
		std::cout
			<< "### Options: \n"
			<< " 1. Integer tree.\n"
			<< " 2. String tree.\n"
			<< " 3. Set Output Text.\n"
			<< " 4. Set Output CSV.\n"
			<< " 5. Change benchmark memory GBs.\n"
			<< " 6. Run full benchmark.\n"
			<< " 7. Run benchmark @ 128.\n"
			<< " 8. Run benchmark @ 338.\n"
			<< " 0. Exit\n"
			<< "Select: ";
		std::cin >> option;
	} while (option > 8);

	std::cout << "\n";
	return option;
}

void interactiveHelp() {
	std::cout
		<< "COMMANDS: \n"
		<< "add [key] [val] -- Add / set an element in the tree.\n"
		<< "set [key] [val] -- Add / set an element in the tree.\n"
		<< "get [key] -- Search for en element in the tree.\n"
		<< "del [key] -- Delete an element from the tree.\n"
		<< "random [size] -- add [size] amount of random elements.\n"
		<< "clear   -- clear the tree.\n"
		<< "show    -- print all key value pairs.\n"
		<< "print   -- alias 'show'.\n"
		<< "iterate -- alias 'show'.\n"
		<< "dot   -- print dot language representation of the tree.\n"
		<< "graph -- alias dot.\n"
		<< "info  -- show tree info.\n"
		<< "help  -- this text.\n"
		<< "quit / exit -- return to main menu.\n"
		<< "\n\n";
}

template<unsigned int Size>
void addPreset(Tree<int, int, Size>& tree, int count) {
	rd::setMax(100);
	for (int i = 0; i < count; ++i) {
		int number = rd::get() * 2;
		tree.set(number, new int(number));
	}
}

template<unsigned int Size>
void addPreset(Tree<std::string, std::string, Size>& tree, int count) {
	rd::setMax(26);

	std::string key = "";
	
	for (int i = 0; i < count; ++i) {
		key = "";
		for (int j = 0; j < 4; ++j) {
			char c = rd::get();
			key += c + 'a';
		}
		tree.set(key, new std::string(key));
	}
}

template<typename Key, typename Val, unsigned int Size>
void interactiveTreeTemplate() {
	Tree<Key, Val, Size> tree;

	interactiveHelp();

	std::string command;
	Key key;
	Val val;

	for (;;) {
		std::cout << "> ";

		std::cin >> command;

		if (command == "dot" || command == "graph") {
			std::cout << "\n";
			tree.dotPrint();
			std::cout << "\n";
		}
		else if (command == "add" || command == "set") {
			std::cin >> key >> val;
			tree.set(key, new Val(val));
		}
		else if (command == "get") {
			std::cin >> key;

			Val* element;
			if (tree.get(key, element)) {
				std::cout << "Found key: " << key << " with value: " << *element << "\n";
			}
			else {
				std::cout << "Key not found.\n";
			}
		}
		else if (command == "del") {
			std::cin >> key;

			Val* element;
			if (tree.removePop(key, element)) {
				std::cout << "Removed key: " << key << " with value: " << *element << "\n";
			}
			else {
				std::cout << "Key not found.\n";
			}
		}
		else if (command == "random") {
			int number = 0;
			if (std::cin >> number && number > 0) {
				addPreset(tree, number);
				std::cout << "Added " << number << " elements.\n";
			}
			else {
				std::cout << "Wrong count.\n";
				std::cin.ignore();
			}
		}
		else if (command == "print" ||command == "show" || command == "iterate") {
			for (Iterator<Key, Val, Size> it = tree.first(); it.isValid(); ++it) {
				std::cout << std::right << std::setw(12) << it.key() << ": " << std::left << *it.value() << "\n";
			}
		}
		else if (command == "info") {
			std::cout << "Tree [\n"
				"  Elements: " << tree.size() << "\n"
				"  Height: " << tree.height << "\n" 
				"  Nodes: " << tree.nodes << "\n"
				<< "]\n";
		}
		else if (command == "clear") {
			tree.clearDelete();
		}
		else if (command == "quit" || command == "exit") {
			tree.clearDelete();
			std::cout << "\n";
			return;
		}
		else if (command == "help") {
			interactiveHelp();
		}
		else {
			std::cout << "Command error. Use 'help' for commands.\n";
			std::cin.ignore();
		}
		std::cin.clear();
	}
}


void interactiveMode() {
	int BenchGBs = 1;
	int choice;

	do {
		choice = readOption();
		switch (choice) {
		case 1:
			interactiveTreeTemplate<int, int, 4>();
			break;
		case 2:
			interactiveTreeTemplate<std::string, std::string, 4>();
			break;
		case 3:
			bench.SetTextOutput(true);
			std::cout << "Output set to Text.\n";
			break;
		case 4:
			bench.SetTextOutput(false);
			std::cout << "Output set to CSV.\n";
			break;
		case 5:
			std::cout << "Current: " << BenchGBs << " GBs. Enter new value: ";
			if (std::cin >> BenchGBs) {
				std::cout << "Bench memory set to: " << BenchGBs << " GBs.\n";
			}
			else {
				std::cout << "Wrong input. Value unchanged.\n";
				std::cin.clear();
				std::cin.ignore();
			}
			break;
		case 6:
			runFullBench(BenchGBs);
			break;
		case 7:
			Benchmark<128>::runTest(BenchGBs);
			break;
		case 8:
			Benchmark<338>::runTest(BenchGBs);
			break;
		}
	} while (choice > 0);
}
#endif