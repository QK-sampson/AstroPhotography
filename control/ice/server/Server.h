/*
 * Server.h -- a class that consolidates the server functions
 *
 * (c) 2016 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _Server_h
#define _Server_h

#include <AstroConfig.h>
#include <AstroLoader.h>
#include <AstroImage.h>
#include <AstroDiscovery.h>
#include <ImageDirectory.h>
#include <AstroTask.h>
#include <AstroGuiding.h>
#include <Ice/Ice.h>

namespace snowstar {

class Server {
	Ice::CommunicatorPtr	ic;

	astro::module::Repository	repository;
	astro::module::Devices		devices;
	astro::persistence::Database	database;
	astro::guiding::GuiderFactory	guiderfactory;
	astro::task::TaskQueue		taskqueue;
	astro::image::ImageDirectory	imagedirectory;

	astro::discover::ServicePublisherPtr	sp;
        astro::discover::ServicePublisherPtr	sps;

	Ice::ObjectAdapterPtr	adapter;

	void	get_configured_services(astro::discover::ServicePublisherPtr sp);

	void	add_devices_servant();
	void	add_event_servant();
	void	add_configuration_servant();
	void	add_images_servant();
	void	add_tasks_servant();
	void	add_instruments_servant();
	void	add_repository_servant();
	void	add_guiding_servant();
	void	add_focusing_servant();
public:
	Server(Ice::CommunicatorPtr ic, const std::string& dbfilename);
	void	waitForShutdown();
};

} // namespace snowstar

#endif /* _Server_h */
