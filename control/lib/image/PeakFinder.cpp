/*
 * PeakFinder.cpp -- find the maximum in an image
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroImage.h>
#include <AstroFilter.h>
#include <AstroDebug.h>
#include <AstroFormat.h>

namespace astro {
namespace image {
namespace filter {

/**
 * \brief Create a new PeakFinder object
 *
 * The radius must be at least 2, smaller radii don't make sense
 */
PeakFinder::PeakFinder(const ImagePoint& approximate, int radius)
	: _approximate(approximate), _radius(radius) {
	if (_radius < 5) {
		std::string	cause
			= stringprintf("peak finder radius %d too small",
				_radius);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw std::range_error(cause);
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "peak finder looking for maximum %s",
		_approximate.toString().c_str());
}

/**
 * \brief Count the number of pixels above v
 */
int	PeakFinder::above(const ConstImageAdapter<double>& image, double v) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "count pixels above %g", v);
	int	counter = 0;
	for (int x = -_radius; x <= _radius; x++) {
		for (int y = -_radius; y <= _radius; y++) {
			double	d = hypot(x, y);
			if (d > _radius) {
				continue;
			}
			if (image.pixel(x, y) > v) {
				counter++;
			}
		}
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%d pixels > %g", counter, v);
	return counter;
}

/**
 * \brief Find a threshold that gives a reasonable number of pixels
 *
 * To compute the centroid, we have to include a reasonable number of
 * pixels around the maximum, between 25 and 49. This method uses interval
 * division to determine such a value.
 */
double	PeakFinder::threshold(const ConstImageAdapter<double>& image,
		double minvalue, double maxvalue) {
	// now start looking for a value so that between 25 and 49 pixels
	// in the area defined by the radius
	// XXX these fixed numbers are a problem, because large stars may need
	// XXX more pixels for a reasonable centroid
	//const int maxpixelcount = _radius * _radius / 4;
	const int maxpixelcount = 49;
	const int minpixelcount = 25;
	const int targetcount = (minpixelcount + maxpixelcount) / 2;

	// now search for a value of the lower limit so that 
	double	vlow = minvalue;
	double	vhigh = maxvalue;
	int	nlow = M_PI * _radius * _radius / 2;
	int	nhigh = 1;
	int	iterations = 32;
	int	pixels = 0;
	double	v = 0;
	do {
		// subdivide the interval
		v = (vlow + vhigh) / 2;
		// count the number of pixels above this level
		pixels = above(image, v);
		if ((pixels > minpixelcount) && (pixels < maxpixelcount)) {
			debug(LOG_DEBUG, DEBUG_LOG, 0,
				"FINAL threshold %g gives %d pixels (%d)",
				v, pixels, 32 - iterations);
			return v;
		}
		double	vnew = (vlow + vhigh) / 2;
		if (pixels < targetcount) {
			vhigh = vnew;
			nhigh = pixels;
		}
		if (pixels > targetcount) {
			vlow = vnew;
			nlow = pixels;
		}
		debug(LOG_DEBUG, DEBUG_LOG, 0, "values %f:%f pixelcount: %d:%d",
			vlow, vhigh, nlow, nhigh);
	} while (iterations--);

	// if we get to this point, then we did not find an optimal level,
	// so we have to be content with a less optimal choice. If the number
	// of pixels is positive and less than half the area of the image,
	// we run with it anyway
	if ((pixels > 0) && (pixels < image.getSize().getPixels() / 3)) {
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"working with suboptimal level %.3f", v);
		return v;
	}
	// now if even that does not work, give up
	std::string	cause = stringprintf("no suitable level found");
	debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
	throw std::runtime_error(cause);
}

/**
 * \brief Compute the centroid of the values above the threshold
 */
std::pair<Point, double>	PeakFinder::centroid(
					const ConstImageAdapter<double>& image,
					double threshold) {
	int	counter = 0;
	double	xsum = 0, ysum = 0, totalweight = 0;
	for (int x = -_radius; x <= _radius; x++) {
		for (int y = -_radius; y <= _radius; y++) {
			if (hypot(x, y) <= _radius) {
				double	v = image.pixel(x, y);
				if (v > threshold) {
					xsum += v * x;
					ysum += v * y;
					totalweight += v;
					counter++;
				}
			}
		}
	}
	Point	result(xsum / totalweight, ysum / totalweight);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "%d points averaged to %s", counter,
		result.toString().c_str());
	return std::make_pair(result, totalweight);
}

/**
 * \brief find the maximum in an image
 */
Point	PeakFinder::operator()(const ConstImageAdapter<double>& image) {
	return peak(image).first;
}

/**
 * \brief Find the peak and its weight
 */
std::pair<Point, double>	PeakFinder::peak(const ConstImageAdapter<double>& image) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "looking for peak in %s image",
		image.getSize().toString().c_str());
	// first get the pixel with the largest value
	Max<double, double>	maxfilter;
	double	maxvalue = maxfilter(image);
	if (_approximate == ImagePoint()) {
		_approximate = maxfilter.getPoint();
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "find peak near %s",
		_approximate.toString().c_str());

	// find the minimum value in a circle of radius _radius arround
	// the maximum, we use a tiling adapter so that we don't have
	// to worry about accessing points outside the image
	adapter::TilingAdapter<double>	ta(image, _approximate);
	
	// we now search in a rectangle of width 2*_radius+1 around the
	// point with the maximum value
	double	minvalue = maxvalue;
	int	counter = 0;
	for (int x = -_radius; x <= _radius; x++) {
		for (int y = -_radius; y <= _radius; y++) {
			double	d = hypot(x, y);
			if (d > _radius) {
				continue;
			}
			counter++;
			if (d <= _radius) {
				double	v = ta.pixel(x, y);
				if (v < minvalue) {
					minvalue = v;
				}
			}
		}
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "radius cicle contains %d pixels",
		counter);
	
	// now find a threshold level that gives a reasonable number
	// of contributing pixels
	double	v = threshold(ta, minvalue, maxvalue);

	// compute the centroid around this point
	std::pair<Point, double>	c = centroid(ta, v);

	// done
	Point	result = Point(_approximate) + c.first;
	debug(LOG_DEBUG, DEBUG_LOG, 0, "found peak: %s, weight = %f",
		result.toString().c_str(), c.second);
	return std::make_pair(result, c.second);
}

} // namespace filter
} // namespace image
} // namespace astro
