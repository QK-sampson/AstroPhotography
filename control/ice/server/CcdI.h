/*
 * CcdI.h -- ICE CCD wrapper class definition
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _CcdI_h
#define _CcdI_h

#include <camera.h>
#include <AstroCamera.h>
#include <DeviceI.h>

namespace snowstar {

/**
 * \brief Sink class for the stream mode
 */
class CcdSink : public astro::camera::ImageSink {
	ImageSinkPrx	sinkprx;
public:
	CcdSink(const Ice::Identity& identity, const Ice::Current& current);
	void	operator()(const astro::camera::ImageQueueEntry& entry);
	void	stop();
};
typedef std::shared_ptr<CcdSink>	CcdSinkPtr;

/**
 * \brief Ccd servant implementation
 */
class CcdI : virtual public Ccd, virtual public DeviceI {
	astro::camera::CcdPtr	_ccd;
	time_t	laststart;
	astro::image::ImagePtr	image;
public:
	CcdI(astro::camera::CcdPtr ccd);
	virtual	~CcdI();

static	CcdPrx	createProxy(const std::string& ccdname,
		const Ice::Current& current);

	// interface methods
	CcdInfo	getInfo(const Ice::Current& current);
	void	startExposure(const Exposure&, const Ice::Current& current);
	ExposureState	exposureStatus(const Ice::Current& current);
	int	lastExposureStart(const Ice::Current& current);
	void	cancelExposure(const Ice::Current& current);
	Exposure	getExposure(const Ice::Current& current);
	ImagePrx	getImage(const Ice::Current& current);
	bool	hasGain(const Ice::Current& current);
	Interval	gainInterval(const Ice::Current& current);
	bool	hasShutter(const Ice::Current& current);
	ShutterState	getShutterState(const Ice::Current& current);
	void	setShutterState(ShutterState, const Ice::Current& current);

	// cooler methods
	bool	hasCooler(const Ice::Current& current);
	CoolerPrx	getCooler(const Ice::Current& current);

	// stream methods
private:
	CcdSinkPtr	_sink;
public:
	void	registerSink(const Ice::Identity& sinkidentity,
			const Ice::Current& current);
	void	startStream(const ::snowstar::Exposure& e, 
			const Ice::Current& current);
	void	updateStream(const ::snowstar::Exposure& e,
			const Ice::Current& current);
	void	stopStream(const ::Ice::Current& current);
	void	unregisterSink(const ::Ice::Current& current);
};

} // namespace snowstar

#endif /* _CcdI_h */
