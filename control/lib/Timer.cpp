/*
 * Timer.cpp -- implement Timer class
 *
 * (c) 2013 Prof Dr Andreas Mueller, HOchschule Rapperswil
 */
#include <AstroUtils.h>
#include <includes.h>

namespace astro {

Timer::Timer() {
	startTime = endTime = 0;
}

double	Timer::gettime() {
	struct timeval	t;
	gettimeofday(&t, NULL);
	return t.tv_sec + 0.000001 * t.tv_usec;
}

void	Timer::start() {
	startTime = gettime();
}

void	Timer::end() {
	endTime = gettime();
}

double	Timer::elapsed() {
	return endTime - startTime;
}

} // namespace astro