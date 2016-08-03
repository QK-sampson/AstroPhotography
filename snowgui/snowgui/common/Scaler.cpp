/*
 * Scaler.cpp -- scaling transformation for images
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <Scaler.h>

namespace snowgui {

void	Scaler::setup(double bottommargin) {
	if (_maxx == _minx) {
		_maxx = _minx + 1;
	}
	if (_maxy == _miny) {
		_maxy = _miny + 1;
	}
	_scalex = (_width - 6) / (_maxx - _minx);
	_scaley = (_height - 6 - bottommargin) / (_maxy - _miny);
	_bottommargin = bottommargin;
}

Scaler::Scaler(double width, double height, double bottommargin) {
	_minx = 0; _maxx = width;
	_miny = 0; _maxy = height;
	_width = width; _height = height;
	setup(bottommargin);
}

Scaler::Scaler(double minx, double maxx, double miny, double maxy,
	double width, double height, double bottommargin)
	: _minx(minx), _maxx(maxx), _miny(miny), _maxy(maxy),
	  _width(width), _height(height) {
	setup(bottommargin);
}

double	Scaler::x(double _x) const {
	return 3 + _scalex * (_x - _minx);
}

double	Scaler::y(double _y) const {
	return _height - 1 - (3 + _scaley * (_y - _miny) + _bottommargin);
}

QPoint	Scaler::operator()(double _x, double _y) const {
	return QPoint(x(_x), y(_y));
}

QPoint	Scaler::operator()(const QPoint& p) const {
	return (*this)(p.x(), p.y());
}

QPoint	Scaler::inverse(const QPoint& p) const {
	double	x = _minx + (p.x() - 3.) / _scalex;
	double	y = _miny + (_height - 4 - (p.y() - _bottommargin)) / _scaley;
	return QPoint(x, y);
}

Scaler::pointlist	Scaler::listWithPosition(
				const std::vector<FocusPoint>& fpv) const {
	pointlist	result;
	std::vector<FocusPoint>::const_iterator	i;
	for (i = fpv.begin(); i != fpv.end(); i++) {
		QPoint	p(i->position(), i->focusvalue());
		result.push_back((*this)(p));
	}
	return result;
}

Scaler::pointlist	Scaler::listWithSequence(
				const std::vector<FocusPoint>& fpv) {
	pointlist	result;
	std::vector<FocusPoint>::const_iterator	i;
	for (i = fpv.begin(); i != fpv.end(); i++) {
		QPoint	p(i->sequence(), i->focusvalue());
		result.push_back((*this)(p));
	}
	return result;
}

std::string	Scaler::toString() const {
	return astro::stringprintf("minx = %f, maxx = %f, "
		"miny = %f, maxy = %f, width = %f, height = %f",
		_minx, _maxx, _miny, _maxy, _width, _height);
}

} // namespace snowgui