#ifndef __TESTBENCH_H_
#define __TESTBENCH_H_

#include <iostream>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <string>
#include <set>
#include <sstream>

#ifdef USE_CHRONO
#include <chrono>
namespace ch = std::chrono;
#else 
#include <sys/time.h>
#endif


// All the data our Benchmark should store for 1 test.
struct TestData {
	long long Time;
	int Steps;

	TestData() 
		: Time(0)
		, Steps(0) {}

	TestData(long long time, int steps)
		: Time(time)
		, Steps(steps) {}
};

void operator+=(TestData& rhs, const TestData& lhs) {
	rhs.Time += lhs.Time;
	rhs.Steps += lhs.Steps;
}

bool operator<(TestData& rhs, const TestData& lhs) {
	if (rhs.Time == lhs.Time) {
		return rhs.Steps < lhs.Steps;
	}
	return rhs.Time < lhs.Time;
}

// Struct to hold the benchmark results.
// Implementation can switch between chrono / unix time through defining USE_CHRONO.

struct Benchmark {

private:
	std::vector<TestData> DijkTime;
	std::vector<TestData> AstarTime;

#ifdef USE_CHRONO
	ch::time_point<ch::system_clock> StartTime;
	void RestartTimer() {
		StartTime = ch::system_clock::now();
	}

	long long GetCurrent() const {
		return ch::duration_cast<ch::microseconds>(ch::system_clock::now() - StartTime).count();
	}
#else
	struct timeval StartTime;

	timeval GetUnixMicros() const {
		struct timeval TimeVal;
		struct timezone TimeZone;
		gettimeofday(&TimeVal, &TimeZone);
		return TimeVal;
	}

	void RestartTimer() {
		StartTime = GetUnixMicros();
	}

	long long GetCurrent() const {
        struct timeval EndTime = GetUnixMicros();
		if (EndTime.tv_sec == StartTime.tv_sec) {
			return EndTime.tv_usec - StartTime.tv_usec;
		}
		return (EndTime.tv_sec - StartTime.tv_sec - 1) * 1000000 + (1000000 - StartTime.tv_usec) + EndTime.tv_usec;
	}
#endif

public:

	// Internal,  formats and prints a line with 2 times and their difference.
	void PrintBenchLine(TestData DijkT, TestData AstarT) {
		static std::string TimestepStr = " micros";
		std::cout << std::right;
		std::cout << "\n\t| Dijk: " << std::setw(7) << DijkT.Time << TimestepStr << " | " << std::setw(3) << DijkT.Steps << " steps"
				 << "\n\t| A*  : " << std::setw(7) << AstarT.Time << TimestepStr << " | " << std::setw(3) << AstarT.Steps << " steps";

		long long Hi = std::max(DijkT.Time, AstarT.Time);
		long long Lo = std::max(std::min(DijkT.Time, AstarT.Time), 1LL);
		
		int Percent = (int)std::floor(((float)Hi / Lo) * 100.f + 0.5f) - 100;
		long long AbsDiff = Hi - Lo;

		std::string Who = DijkT < AstarT ? "Dijk" : "A* ";
		std::cout << "\n\t| " << Who << " is faster by: " << std::setw(3) << Percent << "% ( " << AbsDiff << TimestepStr << " ) A* did " << (DijkT.Steps - AstarT.Steps) << " less steps.\n" ;
	}

public:

	void Reset() {
		DijkTime.clear();
		AstarTime.clear();
	}

	void StartTest() {
		RestartTimer();
	}

	// The Dijkstra test has finished
	// does not start the timer instantly.
	void SwitchTest(int Steps) {
		long long Duration = GetCurrent();
		DijkTime.push_back(TestData(Duration, Steps));
	}

    void StopTest(int Steps) {
		long long Duration = GetCurrent();
		AstarTime.push_back(TestData(Duration, Steps));
	}

	// Print the last added test.
	void PrintLast() {
		size_t Index = DijkTime.size() - 1;
		PrintBenchLine(DijkTime[Index], AstarTime[Index]);
	}

	// Calculate and print total stats.
	void Print() {
        bool ContainsInvalidResult = false;
		TestData DijkTotal;
		TestData AstarTotal;
		for (int i = 0; i < DijkTime.size(); ++i) {
			DijkTotal += DijkTime[i];
			AstarTotal += AstarTime[i];
		}

		std::cout << "Totals:";
		PrintBenchLine(DijkTotal, AstarTotal);
        std::cout << "\n";
	}
};

#endif //__TESTBENCH_H_