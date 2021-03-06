/*
 * SimUtil.cpp -- utilites for the simulator
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <SimUtil.h>
#include <includes.h>
#include <AstroDebug.h>

namespace astro {
namespace camera {
namespace simulator {

static double	advance = 0.;

double	simtime() {
        struct timeval  tv;
        gettimeofday(&tv, NULL);
        double  result = tv.tv_sec;
        result += 0.000001 * tv.tv_usec;
	result += advance;
        return result;
}

void	simtime_advance(double delta) {
	advance += delta;
}

} // namespace simulator
} // namespace camera
} // namespace astro
