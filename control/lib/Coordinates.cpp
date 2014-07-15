/*
 * Coordinates.cpp -- coordinate system implementation
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroCoordinates.h>
#include <cmath>

namespace astro {

void	Angle::reduce() {
	while (_angle > 2 * M_PI) {
		_angle -= 2 * M_PI;
	}
	while (_angle < 0) {
		_angle += 2 * M_PI;
	}
}

Angle::Angle(double angle) : _angle(angle) {
	reduce();
}

double	Angle::degrees() const {
	return 180 * _angle / M_PI;
}

void	Angle::degrees(double degrees) {
	_angle = M_PI * degrees / 180;
}

double	Angle::hours() const {
	return 12 * _angle / M_PI;
}

void	Angle::hours(double hours) {
	_angle = M_PI * hours / 12;
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

} // namespace astro
