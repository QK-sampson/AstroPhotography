/*
 * ThresholdExtractor.cpp -- class to extract a suitable threshold from image
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroAdapter.h>
#include <AstroFilter.h>

using namespace astro::image::filter;
using namespace astro::adapter;

namespace astro {

double	ThresholdExtractor::threshold(const ConstImageAdapter<double>& image) const {
	double	min = Min<double, double>().filter(image);
	double	max = Max<double, double>().filter(image);
	double	span = max - min;

	double	level = (min + max) / 2;
	// XXX real implementation missing

	return level;
}

} // namespace astro
