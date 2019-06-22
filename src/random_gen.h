#ifndef __RANDOM_GEN_H_
#define __RANDOM_GEN_H_

#ifdef CPP17
#include <random>
namespace rd {
	std::mt19937 gen;
	std::uniform_int_distribution<> dis;

	void setMax(unsigned int newMax) {
		dis = std::uniform_int_distribution<>(0, newMax - 1);
	}

	void seed(int newSeed) {
		gen.seed(newSeed);
	}

	unsigned int get() {
		return dis(gen);
	}
}
#else // assume posix
namespace rd {
	unsigned int max = 0;

	void setMax(unsigned int newMax) {
		max = newMax;
	}

	void seed(int newSeed) {
		srand48(newSeed);
	}

	unsigned int get() {
		return lrand48() % max;
	}
}
#endif // CPP17

#endif //__RANDOM_GEN_H_