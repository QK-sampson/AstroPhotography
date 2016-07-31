#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <AstroDiscovery.h>
#include <QLabel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	const astro::discover::ServiceObject	_serviceobject;
	QLabel	*serviceLabel(astro::discover::ServiceSubset::service_type t);
	void	setServiceLabelEnabled(astro::discover::ServiceSubset::service_type t);
public:
	explicit MainWindow(QWidget *parent,
		const astro::discover::ServiceObject serviceobject);
	~MainWindow();

public slots:
	void	launchPreview();

private:
	Ui::MainWindow *ui;

	QMenu	*fileMenu;

	QAction	*connectAction;
	void	connectFile();

	void	createActions();
	void	createMenus();
};

#endif // MAINWINDOW_H