/*
 * CoolerActive.h -- widget to display the cooler's activity
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _CoolerActive_h
#define _CoolerActive_h

#include <QWidget>
#include <QEvent>

namespace snowgui {

class CoolerActive : public QWidget {
	Q_OBJECT

	bool	_active;
	double	_value;
public:
	explicit CoolerActive(QWidget *parent = NULL);
	virtual ~CoolerActive();

	bool	active() const { return _active; }
	bool	value() const { return _value; }
	void	setActive(bool b);
	void	setValue(double v);
	void	paintEvent(QPaintEvent *event);
	void	draw();
	void    update(float actualtemp, float settemp, bool active);

public slots:
	void	update();
};

} // namespace snowgui

#endif /* _CoolerActive_h */
