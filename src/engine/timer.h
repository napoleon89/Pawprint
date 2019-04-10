#ifndef TIMER_H
#define TIMER_H

#include <engine/std.h>

struct Platform;

class Timer
{
	private:
		u64 start_time;
		static u64 perf_count_freq;
		
	public:
		Timer(Platform *platform);
		u64 start(Platform *platform);
		u64 getWallClock(Platform *platform);
		f32 getSecondsElapsed(Platform *platform);
		f32 getMillisecondsElapsed(Platform *platform);
};

#endif // TIMER_H