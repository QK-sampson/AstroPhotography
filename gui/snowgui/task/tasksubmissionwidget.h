/*
 * tasksubissionwidget.h -- class to take submissions to the task queue
 *
 * (c) 2016 Prof Dr Andreas Müller, Hochschule Rapperswil
 */
#ifndef SNOWGUI_TASKSUBMISSIONWIDGET_H
#define SNOWGUI_TASKSUBMISSIONWIDGET_H

#include <InstrumentWidget.h>
#include <device.h>
#include <repository.h>
#include <tasks.h>

namespace snowgui {

namespace Ui {
	class tasksubmissionwidget;
}

class tasksubmissionwidget : public InstrumentWidget {
	Q_OBJECT

	snowstar::CameraPrx	_camera;
	snowstar::RepositoriesPrx	_repositories;
	snowstar::TaskQueuePrx	_tasks;
	std::string	_instrumentname;
	astro::camera::Exposure	_exposure;
	int	_ccdindex;
	int	_coolerindex;
	int	_filterwheelindex;
	int	_mountindex;

public:
	explicit tasksubmissionwidget(QWidget *parent = 0);
	~tasksubmissionwidget();

	virtual void	instrumentSetup(
		astro::discover::ServiceObject serviceobject,
		snowstar::RemoteInstrument instrument);
	void	setRepositories(snowstar::RepositoriesPrx repositories);

public slots:
	void	filterwheelSelected(snowstar::FilterWheelPrx);
	void	submitClicked();
	void	exposureChanged(astro::camera::Exposure);
	void	ccdSelected(int);
	void	coolerSelected(int);
	void	filterwheelSelected(int);
	void	mountSelected(int);

private:
	Ui::tasksubmissionwidget *ui;
};

} // namespace snowgui

#endif // SNOWGUI_TASKSUBMISSIONWIDGET_H
