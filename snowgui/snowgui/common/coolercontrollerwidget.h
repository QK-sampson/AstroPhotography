/*
 * coolercontrollerwidget.h -- Widget to control a cooler
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef COOLERCONTROLLERWIDGET_H
#define COOLERCONTROLLERWIDGET_H

#include <InstrumentWidget.h>

namespace Ui {
	class coolercontrollerwidget;
}

namespace snowgui {

class coolercontrollerwidget : public InstrumentWidget {
	Q_OBJECT

	snowstar::CoolerPrx	_cooler;
public:
	explicit coolercontrollerwidget(QWidget *parent = 0);
	~coolercontrollerwidget();
	void	instrumentSetup(astro::discover::ServiceObject serviceobject,
				snowstar::RemoteInstrument instrument);

signals:
	void	setTemperatureReached();

private:
	void	displayActualTemperature(float actual);
	void	displaySetTemperature(float settemperature);

	void	setupCooler();
	Ui::coolercontrollerwidget *ui;
	QTimer	*statusTimer;

public slots:
	void	setActual();
	void	setSetTemperature(double t);
	void	statusUpdate();
	void	guiChanged();
	void	coolerChanged(int index);
	void	editingFinished();
};

} // namespace snowgui

#endif // COOLERCONTROLLERWIDGET_H