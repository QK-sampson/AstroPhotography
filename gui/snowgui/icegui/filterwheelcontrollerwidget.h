/*
 * filterwheelcontrollerwidget.h -- controller for filterwheel
 *
 * (c) 2016 Prof Dr Andreas Müller, Hochschule Rapperswil
 */
#ifndef FILTERWHEELCONTROLLERWIDGET_H
#define FILTERWHEELCONTROLLERWIDGET_H

#include <InstrumentWidget.h>
#include <QTimer>

namespace snowgui {

namespace Ui {
	class filterwheelcontrollerwidget;
}

/**
 * \brief A reusable component to control a filter wheel
 */
class filterwheelcontrollerwidget : public InstrumentWidget {
	Q_OBJECT

	snowstar::FilterWheelPrx	_filterwheel;
	snowstar::FilterwheelState	_previousstate;
public:
	explicit filterwheelcontrollerwidget(QWidget *parent = 0);
	~filterwheelcontrollerwidget();
	virtual void	instrumentSetup(
		astro::discover::ServiceObject serviceobject,
		snowstar::RemoteInstrument instrument);

signals:
	void	filterInstalled();
	void	filterwheelSelected(snowstar::FilterWheelPrx);
	void	filterwheelSelected(int);

private:
	Ui::filterwheelcontrollerwidget *ui;
	QTimer	statusTimer;

	void	setupFilterwheel();
	void	displayFilter(int index);


public slots:
	void	setFilter(int index);
	void	filterwheelChanged(int);
	void	statusUpdate();
};

} // namespace snowgui

#endif // FILTERWHEELCONTROLLERWIDGET_H
