/*
 * trackingmonitordialog.cpp -- implementation of tracking monitor
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include "trackingmonitordialog.h"
#include "ui_trackingmonitordialog.h"
#include <AstroDebug.h>
#include <AstroFormat.h>

namespace snowgui {

/**
 * \brief Construct a new trackmonitordialog widget
 */
trackingmonitordialog::trackingmonitordialog(QWidget *parent) :
	QDialog(parent), ui(new Ui::trackingmonitordialog) {
	ui->setupUi(this);
}

/**
 * \brief Destroy the trackingmonitordialog
 */
trackingmonitordialog::~trackingmonitordialog() {
	delete ui;
}

/**
 * \brief add a point to the tracks
 */
void	trackingmonitordialog::add(const snowstar::TrackingPoint& point) {
	switch (point.type) {
	case snowstar::ControlGuidePort:
		ui->gpTrack->add(point);
		break;
	case snowstar::ControlAdaptiveOptics:
		ui->aoTrack->add(point);
		break;
	}
}

/**
 * \brief add a tracking history
 */
void	trackingmonitordialog::add(const snowstar::TrackingHistory& history) {
	std::string	title = astro::stringprintf("Track %d", history.trackid);
	setWindowTitle(QString(title.c_str()));
	trackingmonitordialog	*myself = this;
	clearData();

	int	counter = 0;
	std::for_each(history.points.begin(), history.points.end(),
		[myself,&counter](snowstar::TrackingPoint p) {
			myself->add(p);
			counter++;
		}
	);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "added %d points", counter);
}

/**
 * \brief Update the data
 */
void	trackingmonitordialog::updateData() {
	ui->gpTrack->updateData();
	ui->aoTrack->updateData();
}

/**
 * \brief Clear the data
 */
void	trackingmonitordialog::clearData() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "clear requested");
	ui->gpTrack->clearData();
	ui->aoTrack->clearData();
}

/**
 * \brief set the mas per pixel scale for the guide port
 */
void	trackingmonitordialog::gpMasperpixel(double masperpixel) {
	ui->gpTrack->masperpixel(masperpixel);
}

/**
 * \brief set the mas per pixel scale for the adaptive optics port
 */
void	trackingmonitordialog::aoMasperpixel(double masperpixel) {
	ui->gpTrack->masperpixel(masperpixel);
}

} // namespace snowgui
