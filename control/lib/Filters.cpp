/*
 * Filters.cpp -- filters to compute filter values independent of pixel type
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroFilter.h>
#include <AstroFilterfunc.h>

using namespace astro::image;

namespace astro {
namespace image {
namespace filter {

#define	countnans_typed(image, pixel)					\
	{								\
		Image<pixel>	*imagep					\
			= dynamic_cast<Image<pixel> *>(&*image);	\
		if (NULL != imagep) {					\
			if (std::numeric_limits<pixel>::has_quiet_NaN) {\
				CountNaNs<pixel, double>	cn;	\
				return cn.filter(*imagep);		\
			} else {					\
				return 0;				\
			}						\
		}							\
	}

double	countnans(const ImagePtr& image) {
	countnans_typed(image, unsigned char);
	countnans_typed(image, unsigned short);
	countnans_typed(image, unsigned int);
	countnans_typed(image, unsigned long);
	countnans_typed(image, float);
	countnans_typed(image, double);
	throw std::runtime_error("cannot count nans in this image type");
	return 0;
}

#define	filter_typed(image, f, pixel)					\
	{								\
		Image<pixel>	*imagep					\
			= dynamic_cast<Image<pixel> *>(&*image);	\
		if (NULL != imagep) {					\
			f<pixel, double>	m;			\
			return m.filter(*imagep);			\
		}							\
	}

double	mean(const ImagePtr& image) {
	filter_typed(image, Mean, unsigned char);
	filter_typed(image, Mean, unsigned short);
	filter_typed(image, Mean, unsigned int);
	filter_typed(image, Mean, unsigned long);
	filter_typed(image, Mean, float);
	filter_typed(image, Mean, double);
	return 0;
}

#define	filter_typed1(image, f, pixel)				\
	{								\
		Image<pixel>	*imagep					\
			= dynamic_cast<Image<pixel> *>(&*image);	\
		if (NULL != imagep) {					\
			f<pixel, double>	m;			\
			return m.filter(*imagep);			\
		}							\
	}

double	median(const ImagePtr& image) {
	filter_typed1(image, Median, unsigned char);
	filter_typed1(image, Median, unsigned short);
	filter_typed1(image, Median, unsigned int);
	filter_typed1(image, Median, unsigned long);
	filter_typed1(image, Median, float);
	filter_typed1(image, Median, double);
	return 0;
}

#define	filter_typed2(image, f, pixel)					\
	{								\
		Image<pixel>	*imagep					\
			= dynamic_cast<Image<pixel> *>(&*image);	\
		if (NULL != imagep) {					\
			f<pixel, double>	m;			\
			double	v = m.filter(*imagep);			\
			debug(LOG_DEBUG, DEBUG_LOG, 0, "extremum @ %s",	\
				m.getPoint().toString().c_str());	\
			return v;					\
		}							\
	}

double	max(const ImagePtr& image) {
	filter_typed2(image, Max, unsigned char);
	filter_typed2(image, Max, unsigned short);
	filter_typed2(image, Max, unsigned int);
	filter_typed2(image, Max, unsigned long);
	filter_typed2(image, Max, float);
	filter_typed2(image, Max, double);
	return 0;
}

double	min(const ImagePtr& image) {
	filter_typed2(image, Min, unsigned char);
	filter_typed2(image, Min, unsigned short);
	filter_typed2(image, Min, unsigned int);
	filter_typed2(image, Min, unsigned long);
	filter_typed2(image, Min, float);
	filter_typed2(image, Min, double);
	return 0;
}

#define	filter_focusfom(image, pixel, diagonal)				\
	{								\
		Image<pixel>	*imagep					\
			= dynamic_cast<Image<pixel> *>(&*image);	\
		if (NULL != imagep) {					\
			astro::image::filter::FocusFOM<pixel>		\
				m(diagonal);				\
			return m.filter(*imagep);			\
		}							\
	}

double	focusFOM(const ImagePtr& image, const bool diagonal) {
	filter_focusfom(image, unsigned char, diagonal);
	filter_focusfom(image, unsigned short, diagonal);
	filter_focusfom(image, unsigned int, diagonal);
	filter_focusfom(image, unsigned long, diagonal);
	filter_focusfom(image, float, diagonal);
	filter_focusfom(image, double, diagonal);
	return 0;
}

#define	filter_focusfwhm(image, Pixel, center, r)			\
{									\
	Image<Pixel >	*imagep						\
		= dynamic_cast<Image<Pixel > *>(&*image);		\
	if (NULL != imagep) {						\
		FWHM<Pixel>	fwhm(center, r);			\
		return fwhm.filter(*imagep);				\
	}								\
}

double	focusFWHM(const ImagePtr& image, const ImagePoint& center,
	unsigned int r) {
	filter_focusfwhm(image, unsigned char, center, r);
	filter_focusfwhm(image, unsigned short, center, r);
	filter_focusfwhm(image, unsigned int, center, r);
	filter_focusfwhm(image, unsigned long, center, r);
	filter_focusfwhm(image, float, center, r);
	filter_focusfwhm(image, double, center, r);
	return 0;
}

#define	filter_mask(image, pixel, mf)					\
	{								\
		Image<pixel>	*imagep					\
			= dynamic_cast<Image<pixel> *>(&*image);	\
		if (NULL != imagep) {					\
			Mask<pixel>	m(mf);				\
			m(*imagep);					\
		}							\
	}

void	mask(MaskingFunction& maskingfunction, ImagePtr image) {
	filter_mask(image, unsigned char, maskingfunction);
	filter_mask(image, unsigned short, maskingfunction);
	filter_mask(image, unsigned int, maskingfunction);
	filter_mask(image, unsigned long, maskingfunction);
	filter_mask(image, float, maskingfunction);
	filter_mask(image, double, maskingfunction);
}

#define rawvalue_typed(image, Pixel, point)				\
{									\
	Image<Pixel >	*imagep						\
		= dynamic_cast<Image<Pixel > *>(&*image);		\
	if (NULL != imagep) {						\
		return luminance(imagep->pixel(point.x(), point.y()));	\
	}								\
}

#define rawvalue_multi(image, Pixel, point, n)				\
{									\
	Image<Multiplane<Pixel, n> >	*imagep				\
		= dynamic_cast<Image<Multiplane<Pixel, n> > *>(&*image);\
	if (NULL != imagep) {						\
		return luminance(imagep->pixel(point.x(), point.y()));	\
	}								\
}

double	rawvalue(const ImagePtr& image, const ImagePoint& point) {
	rawvalue_typed(image, unsigned char, point);
	rawvalue_typed(image, unsigned short, point);
	rawvalue_typed(image, unsigned int, point);
	rawvalue_typed(image, unsigned long, point);
	rawvalue_typed(image, float, point);
	rawvalue_typed(image, double, point);

	rawvalue_typed(image, RGB<unsigned char>, point);
	rawvalue_typed(image, RGB<unsigned short>, point);
	rawvalue_typed(image, RGB<unsigned int>, point);
	rawvalue_typed(image, RGB<unsigned long>, point);
	rawvalue_typed(image, RGB<float>, point);
	rawvalue_typed(image, RGB<double>, point);

	rawvalue_typed(image, YUYV<unsigned char>, point);
	rawvalue_typed(image, YUYV<unsigned short>, point);
	rawvalue_typed(image, YUYV<unsigned int>, point);
	rawvalue_typed(image, YUYV<unsigned long>, point);
	rawvalue_typed(image, YUYV<float>, point);
	rawvalue_typed(image, YUYV<double>, point);

	rawvalue_multi(image, unsigned char, point, 1);
	rawvalue_multi(image, unsigned short, point, 1);
	rawvalue_multi(image, unsigned int, point, 1);
	rawvalue_multi(image, unsigned long, point, 1);
	rawvalue_multi(image, float, point, 1);
	rawvalue_multi(image, double, point, 1);

	rawvalue_multi(image, unsigned char, point, 2);
	rawvalue_multi(image, unsigned short, point, 2);
	rawvalue_multi(image, unsigned int, point, 2);
	rawvalue_multi(image, unsigned long, point, 2);
	rawvalue_multi(image, float, point, 2);
	rawvalue_multi(image, double, point, 2);

	rawvalue_multi(image, unsigned char, point, 3);
	rawvalue_multi(image, unsigned short, point, 3);
	rawvalue_multi(image, unsigned int, point, 3);
	rawvalue_multi(image, unsigned long, point, 3);
	rawvalue_multi(image, float, point, 3);
	rawvalue_multi(image, double, point, 3);

	rawvalue_multi(image, unsigned char, point, 4);
	rawvalue_multi(image, unsigned short, point, 4);
	rawvalue_multi(image, unsigned int, point, 4);
	rawvalue_multi(image, unsigned long, point, 4);
	rawvalue_multi(image, float, point, 4);
	rawvalue_multi(image, double, point, 4);

	rawvalue_multi(image, unsigned char, point, 5);
	rawvalue_multi(image, unsigned short, point, 5);
	rawvalue_multi(image, unsigned int, point, 5);
	rawvalue_multi(image, unsigned long, point, 5);
	rawvalue_multi(image, float, point, 5);
	rawvalue_multi(image, double, point, 5);

	rawvalue_multi(image, unsigned char, point, 6);
	rawvalue_multi(image, unsigned short, point, 6);
	rawvalue_multi(image, unsigned int, point, 6);
	rawvalue_multi(image, unsigned long, point, 6);
	rawvalue_multi(image, float, point, 6);
	rawvalue_multi(image, double, point, 6);

	rawvalue_multi(image, unsigned char, point, 7);
	rawvalue_multi(image, unsigned short, point, 7);
	rawvalue_multi(image, unsigned int, point, 7);
	rawvalue_multi(image, unsigned long, point, 7);
	rawvalue_multi(image, float, point, 7);
	rawvalue_multi(image, double, point, 7);

	return 0;
}

bool	saturated(const ImagePtr& image, const ImageRectangle& rect) {
	return true;
}

} // namespace filter
} // namespace image
} // namespace astro