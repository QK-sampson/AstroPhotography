/*
 * FilterWheelBusy.cpp
 *
 * (c) 2016 Prof Dr Andreas Müller, Hochschule Rapperswil
 */
#include "FilterWheelBusy.h"
#include <AstroUtils.h>
#include <QPainter>
#include <QEvent>
#include <cmath>

namespace snowgui {

FilterWheelBusy::FilterWheelBusy(QWidget *parent) : QWidget(parent) {
	timer = NULL;
	_nfilters = 5;
	_angle = 0;
}

FilterWheelBusy::~FilterWheelBusy() {
	stop();
}

void	FilterWheelBusy::position(int n) {
	_angle = n * 2 * M_PI / _nfilters;
}

void	FilterWheelBusy::start() {
	_starttime = astro::Timer::gettime() - _angle;
	timer = new QTimer();
        connect(timer, SIGNAL(timeout()), this, SLOT(update()));
        timer->setInterval(50);
	timer->start();
}

void	FilterWheelBusy::stop() {
	if (timer) {
		timer->stop();
		delete timer;
		timer = NULL;
	}
}
	
void	FilterWheelBusy::update() {
	_angle = astro::Timer::gettime() - _starttime;
	repaint();
}

void	FilterWheelBusy::paintEvent(QPaintEvent * /* event */) {
	draw();
}

void	FilterWheelBusy::draw() {
	QPainter	painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// determine center and radius
	QPoint	center(width() / 2., height() / 2.);
	double	radius = std::min(width(), height()) / 2.;

	// background
	QColor	background(0, 0, 0, 0);
	painter.fillRect(0, 0, width(), height(), background);

	// draw the filterwheel 
	QPainterPath	path;
	path.addEllipse(center, radius, radius);
	QColor	black(0, 0, 0);
	painter.fillPath(path, black);
	QColor	white(255, 255, 255);
	QPainterPath	axle;
	axle.addEllipse(center, 2, 2);
	painter.fillPath(axle, white);

	// draw the filters
	double	delta = 2 * M_PI / _nfilters;
	radius = radius - 2;
	double	r = radius / (1 + sin(delta/2));
	if ((2 * r) > (radius - 4)) {
		r = (radius - 4) / 2;
	}
	for (int i = 0; i < _nfilters; i++) {
		double	a = _angle - i * delta;
		QPointF	c(center.x() - (radius - r) * sin(a),
				center.y() - (radius - r) * cos(a));
		QPainterPath	filter;
		filter.addEllipse(c, r, r);
		QColor	color;
		switch (i) {
		case 0: color = white; break;
		case 1:	color = QColor(0, 255, 255); break;
		case 2: color = QColor(255, 0, 255); break;
		case 3: color = QColor(255, 255, 0); break;
		case 4: color = QColor(255, 203, 127); break;
		case 5: color = QColor(195, 131, 131); break;
		case 6: color = QColor(89, 137, 121); break;
		default: color = white; break;
		}
		if (!isEnabled()) {
			// convert the color to an equivalent gray color
			int	gray = (color.red() + color.green()
					+ color.blue()) / 3;
			color = QColor(gray, gray, gray);
		}
		painter.fillPath(filter, color);
	}
}

} // namespace snowgui
