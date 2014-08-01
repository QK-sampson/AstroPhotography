/*
 * Coordinates.cpp -- coordinate system implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroCoordinates.h>
#include <cmath>
#include <stdexcept>
#include <AstroFormat.h>
#include <AstroDebug.h>

namespace astro {

static std::string	xms(double value, const char separator) {
	int	sign = (value >= 0) ? 1 : -1;
	value = fabs(value);
	int	X = floor(value);
	value = 60 * (value - X);
	int	M = floor(value);
	double	S = 60 * (value - M);
	return stringprintf("%02d%c%02d%c%06.3f", sign * X, separator, M,
		separator, S);
}

static double	angle_reduction(const double a, const double base) {
	double	ab = a - base;
	return base + ab - 2 * M_PI * floor(ab / (2 * M_PI));
}

void	Angle::reduce(double base) {
	_angle = angle_reduction(_angle, base);
}

Angle	Angle::reduced(const double base) const {
	return Angle(angle_reduction(_angle, base));
}

double	Angle::degrees_to_radians(const double d) {
	return M_PI * d / 180.;
}

double	Angle::hours_to_radians(const double h) {
	return M_PI * h / 12.;
}

double	Angle::radians_to_hours(const double r) {
	return 12. * r / M_PI;
}

double	Angle::radians_to_degrees(const double r) {
	return 180. * r / M_PI;
}

Angle::Angle(double angle) : _angle(angle) {
	//reduce();
}

double	Angle::degrees() const {
	return radians_to_degrees(_angle);
}

void	Angle::degrees(double degrees) {
	_angle = degrees_to_radians(degrees);
}

std::string	Angle::dms(const char separator) const {
	return xms(degrees(), separator);
}

double	Angle::hours() const {
	return radians_to_hours(_angle);
}

void	Angle::hours(double hours) {
	_angle = hours_to_radians(hours);
}

std::string	Angle::hms(const char separator) const {
	return xms(hours(), separator);
}

Angle	Angle::operator+(const Angle& other) const {
	return Angle(_angle + other._angle);
}

Angle	Angle::operator-(const Angle& other) const {
	return Angle(_angle - other._angle);
}

Angle	Angle::operator*(const double& other) const {
	return Angle(_angle * other);
}

bool	Angle::operator<(const Angle& other) const {
	return _angle < other._angle;
}

bool	Angle::operator<=(const Angle& other) const {
	return _angle <= other._angle;
}

bool	Angle::operator>(const Angle& other) const {
	return _angle > other._angle;
}

bool	Angle::operator>=(const Angle& other) const {
	return _angle >= other._angle;
}

bool	Angle::operator==(const Angle& other) const {
	return 0 == angle_reduction(other._angle, _angle);
}

bool	Angle::operator!=(const Angle& other) const {
	return !((*this) == other);
}

double	cos(const Angle& a) { return ::cos(a.radians()); }
double	sin(const Angle& a) { return ::sin(a.radians()); }
double	tan(const Angle& a) { return ::tan(a.radians()); }
double	cot(const Angle& a) { return 1. / tan(a); }
double	sec(const Angle& a) { return 1 / cos(a); }
double	csc(const Angle& a) { return 1 / sin(a); }

//////////////////////////////////////////////////////////////////////
// TwoAngles implementation
//////////////////////////////////////////////////////////////////////
Angle&	TwoAngles::operator[](int i) {
	switch (i) {
	case 0:	return a1();
	case 1: return a2();
	}
	throw std::range_error("angle index out of range");
}

const Angle&	TwoAngles::operator[](int i) const {
	switch (i) {
	case 0:	return a1();
	case 1: return a2();
	}
	throw std::range_error("angle index out of range");
}

//////////////////////////////////////////////////////////////////////
// Comparison operators for RaDec
//////////////////////////////////////////////////////////////////////
bool	RaDec::operator<(const RaDec& other) const {
	if (dec() < other.dec()) {
		return true;
	}
	if (dec() > other.dec()) {
		return false;
	}
	return ra() < other.ra();
}

bool	RaDec::operator>=(const RaDec& other) const {
	return !(*this < other);
}

bool	RaDec::operator>(const RaDec& other) const {
	if (dec() > other.dec()) {
		return true;
	}
	if (dec() < other.dec()) {
		return false;
	}
	return ra() > other.ra();
}

bool	RaDec::operator<=(const RaDec& other) const {
	return !(*this > other);
}

std::string	RaDec::toString() const {
	return ra().hms() + " " + dec().dms();
}

//////////////////////////////////////////////////////////////////////
// SphericalCoordinates implementation
//////////////////////////////////////////////////////////////////////
SphericalCoordinates::SphericalCoordinates(const LongLat& longlat)
	: TwoAngles(longlat.longitude(), Angle(M_PI / 2) - longlat.latitude()) {
}

SphericalCoordinates::SphericalCoordinates(const RaDec& radec)
	: TwoAngles(radec.ra(), Angle(M_PI / 2) - radec.dec()) {
}

//////////////////////////////////////////////////////////////////////
// UnitVector implementation
//////////////////////////////////////////////////////////////////////
UnitVector::UnitVector(const SphericalCoordinates& spherical) {
	_x[2] = cos(spherical.theta());
	double	r = sin(spherical.theta());
	_x[0] = r * cos(spherical.phi());
	_x[1] = r * sin(spherical.phi());
}

double	UnitVector::operator*(const UnitVector& other) const {
	return _x[0] * other._x[0] + _x[1] * other._x[1] + _x[2] * other._x[2];
}

Angle	UnitVector::operator-(const UnitVector& other) const {
	return Angle(acos(*this * other));
}

Angle	operator-(const SphericalCoordinates& s1, const SphericalCoordinates& s2) {
	return UnitVector(s1) - UnitVector(s2);
}

} // namespace astro
