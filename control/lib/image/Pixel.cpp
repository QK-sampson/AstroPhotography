/*
 * Pixel.cpp -- methods related to pixel types, color conversions
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 * $Id$
 */

#include <AstroPixel.h>
#include <AstroDebug.h>

namespace astro {
namespace image {

/* some auxiliary functions used to convert YUYV to RGB */
static unsigned char    limit(int x) {
	if (x > 255) { return 255; }
	if (x < 0) { return 0; }
	unsigned char   result = 0xff & x;
	return result;
}

static unsigned char    R(int c, int /* d */, int e) {
	return limit((298 * c           + 409 * e + 128) >> 8);
}
static unsigned char    G(int c, int d, int e) {
	return limit((298 * c - 100 * d - 208 * e + 128) >> 8);
}
static unsigned char    B(int c, int d, int /* e */) {
	return limit((298 * c + 516 * d           + 128) >> 8);
}

/**
 * \brief Color conversion from YUYV pixels to RGB pixels.
 */
template<>
void convertPixelPairTyped(RGB<unsigned char> *rgb,
	const YUYV<unsigned char> *yuyv,
	const rgb_color_tag& /* dt */, const yuyv_color_tag& /* ds */) {
	int     c, d, e;
	c = yuyv[0].y - 16;
	d = yuyv[0].uv - 128;
	e = yuyv[1].uv - 128;
	rgb[0].R = R(c, d, e);
	rgb[0].G = G(c, d, e);
	rgb[0].B = B(c, d, e);
	c = yuyv[1].y - 16;
	rgb[1].R = R(c, d, e);
	rgb[1].G = G(c, d, e);
	rgb[1].B = B(c, d, e);
}

/**
 * Auxiliary functions for color conversion from RGB to YUYV
 */
static inline unsigned char    Y(int R, int G, int B) {
        return limit(((  66 * R + 129 * G +  25 * B + 128) >> 8) +  16);
}
static inline unsigned char    U(int R, int G, int B) {
        return limit((( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128);
}
static inline unsigned char    V(int R, int G, int B) {
        return limit((( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128);
}

/**
 * \brief Color conversion from RGB pixels to YUYV pixels.
 */
template<>
void convertPixelPairTyped(YUYV<unsigned char> *yuyv,
	const RGB<unsigned char> *rgb,
	const yuyv_color_tag& /* dt */, const rgb_color_tag& /* ds */) {
	yuyv[0].y  = Y(rgb[0].R, rgb[0].G, rgb[0].B);
	yuyv[0].uv = U(rgb[0].R, rgb[0].G, rgb[0].B);
	yuyv[1].y  = Y(rgb[1].R, rgb[1].G, rgb[1].B);
	yuyv[1].uv = V(rgb[1].R, rgb[1].G, rgb[1].B);
}

/**
 * \brief Specialization for the clipping function
 */
template<>
double	Color<double>::clip(const double& value) {
        return value;
}

template<>
float	Color<float>::clip(const double& value) {
        return value;
}

/**
 * \brief Specializations for the conversion constants
 *
 * These specializations are generated by a macro that needs two arguments.
 * The first argument is the basic type used for the Color class. The
 * second argument is the same in most cases, but for float and double
 * types, which have almost no limits to the size of the pixels, a
 * comparable integer type has to be specified.
 */
#define COLOR_CONSTANTS(T, P)                                           \
template<>                                                              \
const Color<T>::value_type Color<T>::pedestal                           \
        = ((P)16) << ((sizeof(P) - 1) << 3);                            \
template<>                                                              \
const Color<T>::value_type Color<T>::zero                               \
        = ((P)128) << ((sizeof(P) - 1) << 3);

COLOR_CONSTANTS(unsigned char, unsigned char)
COLOR_CONSTANTS(unsigned short, unsigned short)
COLOR_CONSTANTS(unsigned int, unsigned int)
COLOR_CONSTANTS(unsigned long, unsigned long)
/*
 * This choice of P for the floating point types ensures that color
 * space conversion results fromfloating point pixels of type T
 * can be converted without loss to the corresponding * type P,
 * i.e. float -> unsigned int, double -> unsigned long
 */
COLOR_CONSTANTS(float, unsigned int)
COLOR_CONSTANTS(double, unsigned long)

template<>
const Color<unsigned char>::value_type 
        Color<unsigned char>::limit = 0xff;
template<>
const Color<unsigned short>::value_type 
        Color<unsigned short>::limit = 0xffff;
template<>
const Color<unsigned int>::value_type 
        Color<unsigned int>::limit = 0xffffffff;
template<>
const Color<unsigned long>::value_type 
        Color<unsigned long>::limit = 0xffffffffffffffff;
template<>
const Color<float>::value_type 
        Color<float>::limit = 0xffffffff;
template<>
const Color<double>::value_type 
        Color<double>::limit = 0xffffffffffffffff;

template<>
unsigned char   conversionFunction<unsigned char, float>(const float& src) {
	unsigned char	x = src;
	return x;
}

template<>
unsigned char   conversionFunction<unsigned char, double>(const double& src) {
	unsigned char	x = src;
	return x;
}

} // image
} // astro
