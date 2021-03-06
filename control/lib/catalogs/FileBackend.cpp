/*
 * FileBackend.cpp
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil 
 */
#include "CatalogBackend.h"
#include <includes.h>
#include <AstroDebug.h>
#include <AstroFormat.h>
#include "BSC.h"
#include "Hipparcos.h"
#include "Tycho2.h"
#include "Ucac4.h"

namespace astro {
namespace catalog {

/**
 * \brief Create a file based catalog backend
 *
 * The constructor ensures that all the files or directories (in the case of
 * the Ucac4 catalog) are present
 */
FileBackend::FileBackend(const std::string& basedir) : _basedir(basedir) {
	backendname = stringprintf("Files(%s)", basedir.c_str());
	// ensure that the files for the file backend exist
	bsc_catalog = CatalogPtr(new BSC(_basedir
					+ std::string("/bsc")));
	hipparcos_catalog = CatalogPtr(new Hipparcos(_basedir
					+ std::string("/hipparcos")));
	tycho2_catalog = CatalogPtr(new Tycho2(_basedir
					+ std::string("/tycho2")));
	ucac4_catalog = CatalogPtr(new Ucac4(_basedir
					+ std::string("/u4")));
}

/**
 * \brief destroy the backend
 */
FileBackend::~FileBackend() {
}

/**
 * \brief retrieve stars from all catalogs combined
 *
 * This method uses the Hipparcos, Tycho-2 and UCAC4 catalogs in that sequence
 * to get a complete catalog up to magnitude 16. It stops when it can be
 * sure that the catalog is complete. This will happen when the limiting
 * magnitude of the request (the magrange.faintest() value) is smaller than
 * the magnitude up to which the catalog in question is complete. For
 * magnitutes <= 7, the Hipparcos catalog is complete, for magnitudes <= 10,
 * the Tycho-2 catalog is complete with very high probability. 
 */
Catalog::starsetptr	FileBackend::find(const SkyWindow& window,
				const MagnitudeRange& magrange) {
	// prepare the result set
	Catalog::starset	*result = new Catalog::starset;
	Catalog::starsetptr	resultptr(result);

#define	Hipparcos_Complete_Magnitude	7.
#define	Tycho2_Complete_Magnitude	10.

	// if any there are stars requested from the Hipparcos catalog, get them
	{
		// get brightest stars from Hipparcos catalog
		Hipparcos::starsetptr	stars
			= hipparcos_catalog->find(window, magrange);
		Hipparcos::starset::const_iterator	s;
		for (s = stars->begin(); s != stars->end(); s++) {
			Star	star = *s;
			result->insert(star);
		}
	}

	// if the faintest magnitude is brighter than the magnitude to
	// which the Hipparcos catalog is complete, then we can skip 
	// Tacho-2 and UCAC4
	if (magrange.faintest() < Hipparcos_Complete_Magnitude) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "faintest magnitude complete "
			"in Hipparcos catalog");
		return resultptr;
	}

	// get the intermediate stars from the Tycho2 catalog, but skip the
	// stars already retrieved from the Hipparcos catalog
	{
		Tycho2::starsetptr	stars
			= tycho2_catalog->find(window, magrange);
		Tycho2::starset::const_iterator	s;
		for (s = stars->begin(); s != stars->end(); s++) {
			// only take stars not in the Hipparcos catalog and
			// brighter than magnitude 10, as we will get the
			// fainter stars from Ucac4
			if (!s->isDuplicate()) {
				Star	star = *s;
				result->insert(star);
			}
		}
	}

	// if the faintestmagnitude is bright enough so that the Tycho-2
	// catalog is complete up to this magnitude, then we can skip UCAC4
	if (magrange.faintest() < Tycho2_Complete_Magnitude) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "faintest magnitude complete "
			"in Tycho-2 catalog");
		return resultptr;
	}

	// get all matching stars from the UCAC4 catalog
	{
		Ucac4::starsetptr	stars
			= ucac4_catalog->find(window, magrange);
		Ucac4::starset::const_iterator	s;
		for (s = stars->begin(); s != stars->end(); s++) {
			if (!s->isDuplicate()) {
				Star	star = *s;
				result->insert(star);
			}
		}
	}

	return resultptr;
}

CatalogIterator	FileBackend::findIter(const SkyWindow& window,
			const MagnitudeRange& magrange) {
	IteratorImplementationPtr	impl(
		new FileBackendWindowIterator(*this, window, magrange)
	);
	return CatalogIterator(impl);
}

/**
 * \brief Get a star from the unified catalogs by name
 */
Star	FileBackend::find(const std::string& name) {
	// check for BSC stars
	if (name.substr(0, 3) == "BSC") {
		Star	star = bsc_catalog->find(name);
		return star;
	}

	// check for Hipparcos stars
	if (name.substr(0, 3) == "HIP") {
		Star	star = hipparcos_catalog->find(name);
		return star;
	}

	// check for Tycho2 star
	if (name.substr(0, 1) == "T") {
		Star	star = tycho2_catalog->find(name);
		return star;
	}

	// check for Ucac4 star
	if (name.substr(0, 5) == "UCAC4") {
		Star	star = ucac4_catalog->find(name);
		return star;
	}
	throw std::runtime_error("unkonwn star name");
}

unsigned long	FileBackend::numberOfStars() {
	unsigned long	result = 0;
	result += bsc_catalog->numberOfStars();
	result += hipparcos_catalog->numberOfStars();
	result += tycho2_catalog->numberOfStars();
	result += ucac4_catalog->numberOfStars();
	return result;
}

CatalogIterator	FileBackend::begin() {
	IteratorImplementationPtr	impl(new FileBackendIterator(*this));
	return CatalogIterator(impl);
}

} // namespace catalog
} // namespace astro
