/*
 * AstroTransform.h -- transformation of images
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _AstroTransform_h
#define _AstroTransform_h

#include <AstroImage.h>
#include <Format.h>

namespace astro {
namespace image {
namespace transform {

class Point {
public:
	double	x;
	double	y;
	Point() : x(0), y(0) { }
	Point(double _x, double _y) : x(_x), y(_y) { }
	Point(const astro::image::ImagePoint& point) : x(point.x), y(point.y) {}
	Point	operator+(const Point& other) const;
	Point	operator-(const Point& other) const;
	Point	operator*(double l) const;
	friend Point	operator*(double l, const Point& other);
	std::string	toString() const;
	bool	operator==(const Point& other) const;
	bool	operator!=(const Point& other) const;
};
Point	operator*(double l, const Point& other);
std::ostream&	operator<<(std::ostream& out, const Point& other);

/**
 * \brief Abstraction of an affine transform
 */

class Transform {
	double	a[6];
public:
	// constructors
	Transform();
	Transform(const Transform& other);
	Transform(double angle, const Point& translation,
		double scalefactor = 1);
	Transform(const std::vector<Point>& frompoints,
		const std::vector<Point>& topoints);

	// check whether this is a certain type of transform
	bool	isIdentity() const;
	bool	isTranslation() const;
	bool	isRotation() const;
	bool	isHomothety() const;
	bool	isIsometry() const;
	bool	isAreaPreserving() const;
	bool	isAnglePreserving() const;
	bool	fixesOrigin() const;
	bool	operator==(const Transform& other) const;
	bool	operator!=(const Transform& other) const;

	// compute the inverse transformation
	Transform	inverse() const;

	// data access
	Point	getTranslation() const;

	// operations
	Transform	operator*(const Transform& other) const;
	Transform	operator+(const Point& translation) const;
	Transform	operator+(const astro::image::ImagePoint& translation)
				const;

	// operating on points and images
	Point	operator()(const Point& point) const;

	// for debugging
	friend std::ostream&	operator<<(std::ostream& out,
		const Transform& transform);
};

} // namespace transform
} // namespace image
} // namespace astro

#endif /* _AstroTransform_h */
