#ifndef __RANDOM_GEN_H_
#define __RANDOM_GEN_H_

#include <random>

namespace rd {
	std::mt19937 gen;
	std::uniform_int_distribution<> dis;

	void setMax(int newMax) {
		dis = std::uniform_int_distribution<>(0, newMax - 1);
	}

	void seed(int newSeed) {
		gen.seed(newSeed);
	}

	unsigned int get() {
		return dis(gen);
	}
}

#endif //__RANDOM_GEN_H_