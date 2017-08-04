/*
 * singletrackwidget.h -- widget to display a single track
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef SNOWGUI_SINGLETRACKWIDGET_H
#define SNOWGUI_SINGLETRACKWIDGET_H

#include <QWidget>
#include <guider.h>

namespace snowgui {

namespace Ui {
	class singletrackwidget;
}

typedef enum datatype_e { offsetPx, offsetArcsec, correction } datatype_t;

class singletrackwidget : public QWidget {
	Q_OBJECT
	std::vector<snowstar::TrackingPoint>	_points;
	datatype_t	_datatype;
	double	_masperpixel;
public:
	explicit singletrackwidget(QWidget *parent = 0);
	~singletrackwidget();

	void	add(const snowstar::TrackingPoint& point);
	double	masperpixel() const { return _masperpixel; }
	void	masperpixel(double m) { _masperpixel = m; }
	void	updateData();

public slots:
	void	buttonToggled(bool);

private:
	Ui::singletrackwidget *ui;
};

} // namespace snowgui

#endif // SNOWGUI_SINGLETRACKWIDGET_H
