/*
 * TrackingPersistence.h -- Table containing tracking log data
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#ifndef _Tracking_h
#define _Tracking_h

#include <string>
#include <AstroPersistence.h>
#include <AstroGuiding.h>

namespace astro {
namespace guiding {

/**
 * \brief Encapsulation of the information about a guide run
 */
class GuidingRun {
public:
	time_t	whenstarted;
	std::string	camera;
	int	ccdid;
	std::string	guiderport;
	GuidingRun() { }
	GuidingRun(time_t _whenstarted, const std::string& _camera,
		int _ccdid, const std::string& _guiderport)
		: whenstarted(_whenstarted), camera(_camera), ccdid(_ccdid),
		  guiderport(_guiderport) {
	}
};

typedef persistence::Persistent<GuidingRun>	GuidingRunRecord;

/**
 * \brief A class encapsulating a full history, including tracking points
 */
class TrackingHistory : public GuidingRun {
public:
	std::list<TrackingPoint>	points;
	TrackingHistory() { }
	TrackingHistory(const GuidingRun& guidingrun) 
		: GuidingRun(guidingrun) {
	}
};

/**
 * \brief Adapter for GuidingRun table entries
 */
class GuidingRunTableAdapter {
public:
static std::string	tablename();
static std::string	createstatement();
static GuidingRunRecord
	row_to_object(int objectid, const astro::persistence::Row& row);
static astro::persistence::UpdateSpec
	object_to_updatespec(const GuidingRunRecord& guidingrun);
};

typedef astro::persistence::Table<GuidingRunRecord, GuidingRunTableAdapter>	GuidingRunTable;


typedef	persistence::PersistentRef<TrackingPoint>	TrackingPointRecord;

/**
 * \brief Adapter for the Tracking table
 */
class TrackingTableAdapter {
public:
static std::string	tablename();
static std::string	createstatement();
static TrackingPointRecord	row_to_object(int objectid, const astro::persistence::Row& row);
static astro::persistence::UpdateSpec	object_to_updatespec(const TrackingPointRecord& tracking);
};

typedef astro::persistence::Table<TrackingPointRecord, TrackingTableAdapter>	TrackingTable;

} // namespace guiding
} // namespace astro

#endif /* _Tracking_h */
