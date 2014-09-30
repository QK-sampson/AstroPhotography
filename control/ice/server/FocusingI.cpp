/*
 * FocusingI.cpp -- focusing servant implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <FocusingI.h>
#include <IceConversions.h>
#include <Ice/Connection.h>
#include <ProxyCreator.h>
#include <AstroDebug.h>

namespace snowstar {

//////////////////////////////////////////////////////////////////////
// focusing servant implementation
//////////////////////////////////////////////////////////////////////
FocusingI::FocusingI(astro::focusing::FocusingPtr focusingptr) {
	_focusingptr = focusingptr;
	astro::callback::CallbackPtr	cb
		= astro::callback::CallbackPtr(new FocusingCallback(*this));
	_focusingptr->callback(cb);
}

FocusingI::~FocusingI() {
}

FocusState	FocusingI::status(const Ice::Current& /* current */) {
	return convert(_focusingptr->status());
}

FocusMethod	FocusingI::method(const Ice::Current& /* current */) {
	return convert(_focusingptr->method());
}

void	FocusingI::setMethod(FocusMethod method,
		const Ice::Current& /* current */) {
	astro::focusing::Focusing::focus_method	m = convert(method);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set the method to %s",
		astro::focusing::Focusing::name_of_method(m).c_str());
	_focusingptr->method(m);
}


Exposure	FocusingI::getExposure(const Ice::Current& /* current */) {
	return convert(_focusingptr->exposure());
}

void	FocusingI::setExposure(const Exposure& exposure,
		const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set exposure");
	_focusingptr->exposure(convert(exposure));
}


int	FocusingI::steps(const Ice::Current& /* current */) {
	return _focusingptr->steps();
}

void	FocusingI::setSteps(int steps, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "set steps to %d", steps);
	_focusingptr->steps(steps);
}

void	FocusingI::start(int min, int max, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start focusing in interval [%d,%d]",
		min, max);
	// ensure we are in the right state
	switch (_focusingptr->status()) {
	case astro::focusing::Focusing::MOVING:
	case astro::focusing::Focusing::MEASURING:
		throw BadState(std::string("currently focusing"));
	default:
		break;
	}
	try {
		// clear the history
		_history.clear();
		// start the focusing
		_focusingptr->start(min, max);
	} catch (const std::exception& x) {
		throw BadState(std::string("cannot start focusing: ")
			+ std::string(x.what()));
	} catch (...) {
		throw BadState(std::string("cannot start focusing"));
	}
}

void	FocusingI::cancel(const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "cancelling the focusing");
	_focusingptr->cancel();
}


CcdPrx	FocusingI::getCcd(const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "creating the CCD proxy");
	std::string	name = _focusingptr->ccd()->name();
	return createProxy<CcdPrx>(name, current);
}

FocuserPrx	FocusingI::getFocuser(const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "creating the focuser proxy");
	std::string	name = _focusingptr->focuser()->name();
	return createProxy<FocuserPrx>(name, current);
}


FocusHistory	FocusingI::history(const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "retrieve the history");
	return _history;
}

/**
 * \brief send an add point callback to all registered callbacks
 */
void	FocusingI::addPoint(const FocusPoint& point) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "adding a point %d: %f",
		point.position, point.value);
	_history.push_back(point);

	std::list<FocusCallbackPrx>	todelete;
	for (auto ptr = callbackproxies.begin(); ptr != callbackproxies.end();
		ptr++) {
		try {
			(*ptr)->addPoint(point);
		} catch (...) {
			todelete.push_back(*ptr);
		}
	}
	for (auto ptr = todelete.begin(); ptr != todelete.end(); ptr++) {
		callbackproxies.erase(*ptr);
	}
}

/**
 * \brief send a change state callback to all registered callbacks
 */
void	FocusingI::changeState(FocusState state) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "sending new state %d", state);
	std::list<FocusCallbackPrx>	todelete;
	for (auto ptr = callbackproxies.begin(); ptr != callbackproxies.end();
		ptr++) {
		try {
			(*ptr)->changeState(state);
		} catch (...) {
			todelete.push_back(*ptr);
		}
	}
	for (auto ptr = todelete.begin(); ptr != todelete.end(); ptr++) {
		callbackproxies.erase(*ptr);
	}
}

/**
 * \brief register a callback 
 */
void	FocusingI::registerCallback(const Ice::Identity& callbackidentity,
		const Ice::Current& current) {
	FocusCallbackPrx	callback = FocusCallbackPrx::uncheckedCast(
		current.con->createProxy(callbackidentity));
	callbackproxies.insert(callback);
}

/**
 * \brief unregister a callback
 */
void	FocusingI::unregisterCallback(const Ice::Identity& callbackidentity,
		const Ice::Current& current) {
	FocusCallbackPrx	callback = FocusCallbackPrx::uncheckedCast(
		current.con->createProxy(callbackidentity));
	callbackproxies.erase(callback);
}

//////////////////////////////////////////////////////////////////////
// focusing callback
//////////////////////////////////////////////////////////////////////
astro::callback::CallbackDataPtr	FocusingCallback::operator()(
	astro::callback::CallbackDataPtr data) {
	astro::focusing::FocusCallbackData	*focusdata
		= dynamic_cast<astro::focusing::FocusCallbackData *>(&*data);
	if (NULL != focusdata) {
		FocusPoint	p;
		p.position = focusdata->position();
		p.value = focusdata->value();
		_focusing.addPoint(p);
	}
	astro::focusing::FocusCallbackState	*focusstate
		= dynamic_cast<astro::focusing::FocusCallbackState *>(&*data);
	if (NULL != focusstate) {
		_focusing.changeState(convert(focusstate->state()));
	}
	return data;
}

} // namespace snowstar
