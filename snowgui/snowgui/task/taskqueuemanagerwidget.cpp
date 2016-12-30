/*
 * taskqueuemanagerwidget.cpp -- task queue manager implementation
 *
 * (c) 2016 Prof Dr Andreas Müeller, Hochschule Rapperswil
 */
#include "taskqueuemanagerwidget.h"
#include "ui_taskqueuemanagerwidget.h"
#include <CommunicatorSingleton.h>
#include <AstroDebug.h>
#include <AstroCamera.h>
#include <IceConversions.h>

namespace snowgui {

taskqueuemanagerwidget::taskqueuemanagerwidget(QWidget *parent)
	: QWidget(parent), ui(new Ui::taskqueuemanagerwidget) {
	ui->setupUi(this);

	// configure the task list
	QStringList	headers;
	headers << "ID";		//  0
	headers << "Instrument";	//  1
	headers << "Project";		//  2
	headers << "Purpose";		//  3
	headers << "Time";		//  4
	headers << "Exposure";		//  5
	headers << "Filter";		//  6
	headers << "Binning";		//  7
	headers << "Temperature";	//  8
	ui->taskTree->setHeaderLabels(headers);
	ui->taskTree->header()->resizeSection(0, 50);
	ui->taskTree->header()->resizeSection(1, 80);
	ui->taskTree->header()->resizeSection(2, 100);
	ui->taskTree->header()->resizeSection(3, 80);
	ui->taskTree->header()->resizeSection(4, 80);
	ui->taskTree->header()->resizeSection(5, 60);
	ui->taskTree->header()->resizeSection(6, 100);
	ui->taskTree->header()->resizeSection(7, 50);
	ui->taskTree->header()->resizeSection(8, 80);

	// create the top level entries in the tree
	{
		QStringList	list;
		list << "" << "completed";
		QTreeWidgetItem	*item;
		item = new QTreeWidgetItem(list, QTreeWidgetItem::Type);
		ui->taskTree->addTopLevelItem(item);
	}
	{
		QStringList	list;
		list << "" << "cancelled";
		QTreeWidgetItem	*item;
		item = new QTreeWidgetItem(list, QTreeWidgetItem::Type);
		ui->taskTree->addTopLevelItem(item);
	}
	{
		QStringList	list;
		list << "" << "failed";
		QTreeWidgetItem	*item;
		item = new QTreeWidgetItem(list, QTreeWidgetItem::Type);
		ui->taskTree->addTopLevelItem(item);
	}
	{
		QStringList	list;
		list << "" << "executing";
		QTreeWidgetItem	*item;
		item = new QTreeWidgetItem(list, QTreeWidgetItem::Type);
		ui->taskTree->addTopLevelItem(item);
	}
	{
		QStringList	list;
		list << "" << "pending";
		QTreeWidgetItem	*item;
		item = new QTreeWidgetItem(list, QTreeWidgetItem::Type);
		ui->taskTree->addTopLevelItem(item);
	}

	// connect buttons to slots
	connect(ui->infoButton, SIGNAL(clicked()),
		this, SLOT(infoClicked()));
	connect(ui->cancelButton, SIGNAL(clicked()),
		this, SLOT(cancelClicked()));
	connect(ui->imageButton, SIGNAL(clicked()),
		this, SLOT(imageClicked()));
	connect(ui->downloadButton, SIGNAL(clicked()),
		this, SLOT(downloadClicked()));
	connect(ui->deleteButton, SIGNAL(clicked()),
		this, SLOT(deleteClicked()));
}

taskqueuemanagerwidget::~taskqueuemanagerwidget() {
	delete ui;
}

void	taskqueuemanagerwidget::addTasks(QTreeWidgetItem *parent,
		snowstar::TaskState state) {
	snowstar::taskidsequence	s = _tasks->tasklist(state);
	snowstar::taskidsequence::const_iterator	i;
	for (i = s.begin(); i != s.end(); i++) {
		snowstar::TaskInfo	info = _tasks->info(*i);
		snowstar::TaskParameters	parameters
			= _tasks->parameters(*i);
		astro::camera::Exposure	exposure = snowstar::convert(parameters.exp);

		QStringList	list;

		// 0 taskid>
		list << QString::number(info.taskid);

		// 1 instrument
		list << QString(parameters.instrument.c_str());

		// 2 project
		list << QString(parameters.project.c_str());

		// 3 purpose
		list << QString(astro::camera::Exposure::purpose2string(exposure.purpose()).c_str());

		// 4 last state change
		list << "";

		// 5 exposure time
		list << QString(astro::stringprintf("%.3f", exposure.exposuretime()).c_str());

		// 6 filter
		list << QString(parameters.filter.c_str());

		// 7 binning
		std::string	binning = exposure.mode().toString();
		list << QString(binning.substr(1, binning.size() - 2).c_str());

		// 8 temperature
		list << QString(astro::stringprintf("%.1f",
			parameters.ccdtemperature - 273.15).c_str());

		QTreeWidgetItem	*item = new QTreeWidgetItem(list,
			QTreeWidgetItem::Type);
		item->setTextAlignment(0, Qt::AlignLeft);
		item->setTextAlignment(1, Qt::AlignLeft);
		item->setTextAlignment(2, Qt::AlignLeft);
		item->setTextAlignment(3, Qt::AlignLeft);
		item->setTextAlignment(4, Qt::AlignLeft);
		item->setTextAlignment(5, Qt::AlignRight);
		item->setTextAlignment(6, Qt::AlignLeft);
		item->setTextAlignment(7, Qt::AlignLeft);
		item->setTextAlignment(6, Qt::AlignRight);
		parent->addChild(item);
	}
}

void	taskqueuemanagerwidget::addTasks() {
	if (!_tasks) {
		debug(LOG_DEBUG, DEBUG_LOG, 0,
			"no tasks proxy, cannot add tasks");
		return;
	}

	addTasks(ui->taskTree->topLevelItem(0), snowstar::TskCOMPLETE);
	addTasks(ui->taskTree->topLevelItem(1), snowstar::TskCANCELLED);
	addTasks(ui->taskTree->topLevelItem(2), snowstar::TskFAILED);
	addTasks(ui->taskTree->topLevelItem(3), snowstar::TskEXECUTING);
	addTasks(ui->taskTree->topLevelItem(4), snowstar::TskPENDING);
}

void	taskqueuemanagerwidget::setServiceObject(
		astro::discover::ServiceObject serviceobject) {
	// get the Tasks proxy
	Ice::CommunicatorPtr    ic = snowstar::CommunicatorSingleton::get();
	Ice::ObjectPrx  base = ic->stringToProxy(
		serviceobject.connect("Tasks"));
	_tasks = snowstar::TaskQueuePrx::checkedCast(base);
	if (!_tasks) {
		debug(LOG_ERR, DEBUG_LOG, 0, "could not get a taskqueue");
	}

	// get the repositories proxy
	base = ic->stringToProxy(serviceobject.connect("Repositories"));
	_repositories = snowstar::RepositoriesPrx::checkedCast(base);
	if (!_repositories) {
		debug(LOG_ERR, DEBUG_LOG, 0, "could not get the repositories");
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "service setup complete");

	// add the tasks
	addTasks();
}

void	taskqueuemanagerwidget::infoClicked() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "infoClicked()");
}

void	taskqueuemanagerwidget::cancelClicked() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "cancelClicked()");
}

void	taskqueuemanagerwidget::imageClicked() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "imageClicked()");
}

void	taskqueuemanagerwidget::downloadClicked() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "downloadClicked()");
}

void	taskqueuemanagerwidget::deleteClicked() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "deleteClicked()");
}

} // namespace snowgui
