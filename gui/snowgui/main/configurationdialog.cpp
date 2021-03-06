/*
 * configurationdialog.cpp
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include "configurationdialog.h"
#include "ui_configurationdialog.h"
#include <CommunicatorSingleton.h>
#include <sstream>
#include <AstroFormat.h>
#include <AstroDebug.h>
#include <QMessageBox>

namespace snowgui {

/**
 * \brief construct a configuration dialog
 */
configurationdialog::configurationdialog(QWidget *parent,
	astro::discover::ServiceObject serviceobject)
	: QDialog(parent), ui(new Ui::configurationdialog),
	  _serviceobject(serviceobject) {
	ui->setupUi(this);

	_serviceobject = serviceobject;
	_servicechangewarning = false;

	// connect to the server
	Ice::CommunicatorPtr    ic = snowstar::CommunicatorSingleton::get();
	Ice::ObjectPrx  base = ic->stringToProxy(
				_serviceobject.connect("Configuration"));
	snowstar::ConfigurationPrx	configuration = snowstar::ConfigurationPrx::checkedCast(base);
	if (!base) {
		throw std::runtime_error("cannot create configuration app");
	}
	setConfiguration(configuration);

	// find out whether the remote supports 
	try {
		Ice::ObjectPrx	base = ic->stringToProxy(
				_serviceobject.connect("Repositories"));
		snowstar::RepositoriesPrx	repositories
			= snowstar::RepositoriesPrx::checkedCast(base);
		ui->repositoryconfiguration->setRepositories(repositories);
	} catch (...) {
	}

	// connect buttons
	connect(ui->devicesCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(devicesToggled(bool)));
	connect(ui->imagesCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(imagesToggled(bool)));
	connect(ui->instrumentsCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(instrumentsToggled(bool)));
	connect(ui->guidingCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(guidingToggled(bool)));
	connect(ui->focusingCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(focusingToggled(bool)));
	connect(ui->repositoriesCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(repositoriesToggled(bool)));
	connect(ui->tasksCheckBox, SIGNAL(toggled(bool)),
		this, SLOT(tasksToggled(bool)));

	connect(ui->restartButton, SIGNAL(clicked()),
		this, SLOT(restartClicked()));

	// title
	setWindowTitle(QString("Configuration"));
	std::string	title = astro::stringprintf("Remote configuration on %s", _serviceobject.toString().c_str());
	ui->remoteconfigurationLabel->setText(QString(title.c_str()));
}

/**
 * \brief destroy the configuration dialog
 */
configurationdialog::~configurationdialog() {
	delete ui;
}

bool	configurationdialog::getService(const std::string& name) {
	std::string	value("no");
	if ((name == "devices") || (name == "images")) {
		value = std::string("yes");
	}
	snowstar::ConfigurationKey	key;
	key.domain = "snowstar";
	key.section = "service";
	key.name = name;
	if (_configuration->has(key)) {
		snowstar::ConfigurationItem	item
			= _configuration->get(key);
		value = item.value;
	}
	return (value == "yes");
}

template<class T>
class Blocker {
	T	*_blocked;
public:
	Blocker(T *blocked) : _blocked(blocked) {
		_blocked->blockSignals(true);
	}
	~Blocker() { 
		_blocked->blockSignals(false);
	}
	T	*operator->() { return _blocked; }
};

template<class T> inline Blocker<T>	whileBlocking(T *blocked) {
	return Blocker<T>(blocked);
}

/**
 * \brief set a new configuration
 */
void	configurationdialog::setConfiguration(snowstar::ConfigurationPrx configuration) {
	if (!configuration) {
		return;
	}
	_configuration = configuration;
	// read the configuration information and update the widgets
	whileBlocking(ui->devicesCheckBox)->setChecked(getService("devices"));
	whileBlocking(ui->imagesCheckBox)->setChecked(getService("images"));
	whileBlocking(ui->instrumentsCheckBox)->setChecked(getService("instruments"));
	whileBlocking(ui->guidingCheckBox)->setChecked(getService("guiding"));
	whileBlocking(ui->focusingCheckBox)->setChecked(getService("focusing"));
	whileBlocking(ui->repositoriesCheckBox)->setChecked(getService("repository"));
	whileBlocking(ui->tasksCheckBox)->setChecked(getService("tasks"));
}

void	configurationdialog::changevalue(const std::string& name, bool defaultvalue, bool newvalue) {
	std::string	targetvalue = (newvalue) ? "yes" : "no";
	snowstar::ConfigurationKey	key;
	key.domain = "snowstar";
	key.section = "service";
	key.name = name;
	snowstar::ConfigurationItem	item;
	if (_configuration->has(key)) {
		item = _configuration->get(key);
	} else {
		item.domain = "snowstar";
		item.section = "service";
		item.name = name;
		item.value = (defaultvalue) ? "yes" : "no";
	}
	if (targetvalue != item.value) {
		item.value = targetvalue;
		_configuration->set(item);
	}
	if (_servicechangewarning) {
		return;
	}
	ui->restartButton->setEnabled(true);
	QMessageBox	*message = new QMessageBox(this);
	message->setText(QString("Server restart required"));
	std::ostringstream	str;
	str << "Changing the service configuration requires a server restart. ";
	str << "Please exit all Snowstar appications and restart the Snowstar server process on '";
	str << _serviceobject.toString();
	str << "'.";
	message->setInformativeText(QString(str.str().c_str()));
	message->exec();
	delete message;
	_servicechangewarning = true;
}

void	configurationdialog::devicesToggled(bool newvalue) {
	changevalue("devices", true, newvalue);
}

void	configurationdialog::instrumentsToggled(bool newvalue) {
	changevalue("instruments", false, newvalue);
}

void	configurationdialog::imagesToggled(bool newvalue) {
	changevalue("images", true, newvalue);
}

void	configurationdialog::guidingToggled(bool newvalue) {
	changevalue("guiding", false, newvalue);
}

void	configurationdialog::focusingToggled(bool newvalue) {
	changevalue("focusing", false, newvalue);
}

void	configurationdialog::repositoriesToggled(bool newvalue) {
	changevalue("repository", false, newvalue);
}

void	configurationdialog::tasksToggled(bool newvalue) {
	changevalue("tasks", false, newvalue);
}

void	configurationdialog::restartClicked() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "restart initiated");
	_configuration->restartServer(1.0f);
}


} // namespace snowgui
