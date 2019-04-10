#include <engine/timer.h>
#include <core/platform.h>
		
Timer::Timer(Platform *platform) { 
	if(perf_count_freq == 0) 
		perf_count_freq = platform->getPerformanceFrequency(); 
}

u64 Timer::start(Platform *platform) { 
	start_time = getWallClock(platform); 
	return start_time; 
}

u64 Timer::getWallClock(Platform *platform) { 
	return platform->getPerformanceCounter(); 
}

f32 Timer::getSecondsElapsed(Platform *platform) { 
	f32 result = ((f32)(getWallClock(platform) - start_time) / (f32)perf_count_freq); 
	return result; 
}

f32 Timer::getMillisecondsElapsed(Platform *platform) { 
	return getSecondsElapsed(platform) * 1000.0f; 
}


u64 Timer::perf_count_freq = 0;