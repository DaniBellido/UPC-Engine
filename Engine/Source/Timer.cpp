#include "Globals.h"
#include "Timer.h"

Timer::Timer() : running(false)
{
}

void Timer::Start()
{
	start = clock::now();
	running = true;
}

void Timer::Stop()
{
	if (running)
	{
		end = clock::now();
		running = false;
	}
}

double Timer::ReadMs() const
{
	auto t = running ? clock::now() : end;
	return std::chrono::duration<double, std::milli>(t - start).count();
}

double Timer::ReadUs() const
{
	auto t = running ? clock::now() : end;
	return std::chrono::duration<double, std::micro>(t - start).count();
}
