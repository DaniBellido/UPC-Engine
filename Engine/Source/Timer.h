#pragma once
#include <chrono>

class Timer
{
private:
	using clock = std::chrono::steady_clock;

	clock::time_point start;
	clock::time_point end;

	bool running;

public:
	Timer();

	void Start();
	void Stop();

	double ReadMs() const;  // Milliseconds from Start
	double ReadUs() const;  // Microseconds from Start

};

