/*
 * ccdcontrollerwidget.h -- common CCD controller
 *
 * (c) 2016 Prof Dr Andreas Müller, Hochschule Rapperswil
 */
#ifndef CCDCONTROLLERWIDGET_H
#define CCDCONTROLLERWIDGET_H

#include <InstrumentWidget.h>
#include <AstroCamera.h>
#include <image.h>
#include <QTimer>

namespace snowgui {

namespace Ui {
	class ccdcontrollerwidget;
}

/**
 * \brief A reusable component to control a CCD
 */
class ccdcontrollerwidget : public InstrumentWidget {
	Q_OBJECT

	snowstar::CcdPrx	_ccd;
	snowstar::CcdInfo	_ccdinfo;
	astro::camera::Exposure	_exposure;

	astro::image::ImagePtr	_image;
	astro::camera::Exposure	_imageexposure;
	snowstar::ImagePrx	_imageproxy;

	bool	_guiderccdonly;
	bool	_nosubframe;
	bool	_nobuttons;
	bool	_imageproxyonly;

public:
	explicit ccdcontrollerwidget(QWidget *parent = NULL);
	~ccdcontrollerwidget();
	virtual void	instrumentSetup(
		astro::discover::ServiceObject serviceobject,
		snowstar::RemoteInstrument instrument);

	const astro::camera::Exposure&	exposure() const { return _exposure; }
	const astro::image::ImagePtr	image() const { return _image; }
	const astro::camera::Exposure&	imageexposure() const { return _imageexposure; }

	bool	guiderccdonly() const { return _guiderccdonly; }
	void	guiderccdonly(bool g) { _guiderccdonly = g; }

	bool	imageproxyonly() const { return _imageproxyonly; }
	void	imageproxyonly(bool i) { _imageproxyonly = i; }

signals:
	void	exposureChanged(astro::camera::Exposure);
	void	imageReceived(astro::image::ImagePtr image);
	void	imageproxyReceived(snowstar::ImagePrx image);
	void	ccdSelected(int);

private:
	void	setupCcd();
	void	ccdFailed(const std::exception&);

	// methods to synchronize the GUI state with the internal state
	void	displayExposure(astro::camera::Exposure);
	void	displayBinning(astro::image::Binning);
	void	displayBinning(int index);
	astro::image::Binning	getBinning(int index);
	void	displayExposureTime(double);
	astro::camera::Exposure::purpose_t	getPurpose(int index);
	void	displayPurpose(astro::camera::Exposure::purpose_t);
	void	displayShutter(astro::camera::Shutter::state);
	void	displayFrame(astro::image::ImageRectangle);

	// retrieve an image
	void	retrieveImage();

	Ui::ccdcontrollerwidget *ui;
	QTimer	statusTimer;
	snowstar::ExposureState	previousstate;
	bool	ourexposure;

public slots:
	// changes taht come from the outside
	void	setExposure(astro::camera::Exposure);
	void	setBinning(astro::image::Binning);
	void	setExposureTime(double);
	void	setPurpose(astro::camera::Exposure::purpose_t);
	void	setShutter(astro::camera::Shutter::state);
	void	setFrame(astro::image::ImageRectangle);
	void	setSubframe(astro::image::ImageRectangle);
	void	setImage(astro::image::ImagePtr);

	// this slot is used by the GUI elements to update the settings 
	// from the GUI
	void	guiChanged();
	void	captureClicked();
	void	cancelClicked();
	void	streamClicked();
	void	ccdChanged(int);
	void	hideSubframe(bool);
	void	hideButtons(bool);

	// needed internally for status udpates
	void	statusUpdate();

};

} // namespace snowgui

#endif // CCDCONTROLLERWIDGET_H
