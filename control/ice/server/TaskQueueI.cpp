/*
 * TaskQueueI.cpp -- task queue servant implementation
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <TaskQueueI.h>
#include <algorithm>
#include <AstroFormat.h>
#include <sstream>
#include <ProxyCreator.h>
#include <IceConversions.h>
#include <AstroDebug.h>
#include <typeinfo>
#include <AstroEvent.h>

namespace snowstar {

/**
 * \brief Spezialization of the callback_adapter for TaskMonitorPrx
 */
template<>
void	callback_adapter<TaskMonitorPrx>(TaskMonitorPrx& p,
		const astro::callback::CallbackDataPtr data) {
	astro::task::TaskMonitorCallbackData	*tmcd
		= dynamic_cast<astro::task::TaskMonitorCallbackData *>(&*data);

	// if there is no task monitor info, then give up immediately
	if (NULL == tmcd) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no task monitor callback data");
		return;
	}
	const astro::task::TaskMonitorInfo&	ti = tmcd->data();

	// send the information to the clients
	p->update(convert(ti));
	debug(LOG_DEBUG, DEBUG_LOG, 0, "sending update returned");
}

TaskQueueI::TaskQueueI(astro::task::TaskQueue& _taskqueue)
	: taskqueue(_taskqueue) {
	// recover from crashes
	taskqueue.recover();

	// install callback that publishes updates
	TaskQueueICallback	*taskqueuecallback
		= new TaskQueueICallback(*this);
	taskqueue.callback = astro::callback::CallbackPtr(taskqueuecallback);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "task queue callback installed %p",
		taskqueuecallback);

	astro::event(EVENT_CLASS, astro::events::INFO,
		astro::events::Event::TASK,
		"task queue initialized");
}

TaskQueueI::~TaskQueueI() {
}

// interface methods
QueueState TaskQueueI::state(const Ice::Current& /* current */) {
	//debug(LOG_DEBUG, DEBUG_LOG, 0, "state request");
	return convert(taskqueue.state());
}

void TaskQueueI::start(const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "start request");
	try {
		taskqueue.start();
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::TASK,
			"task queue started");
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot start: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw BadState(cause);
	}
}

void TaskQueueI::stop(const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "stop request");
	try {
		taskqueue.stop();
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::TASK,
			"task queue stopped");
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot stop: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw BadState(cause);
	}
}

int TaskQueueI::submit(const TaskParameters& parameters,
		const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "submit a new task on '%s', purpose = %d",
		parameters.instrument.c_str(), parameters.exp.purpose);
	// get information about the parameters
	astro::discover::InstrumentPtr  instrument
                = astro::discover::InstrumentBackend::get(
			parameters.instrument);
	astro::task::TaskInfo	info(-1);
	if ((instrument->nComponentsOfType(
		astro::discover::InstrumentComponentKey::Camera) > 0)
		&& (parameters.cameraIndex >= 0)) {
		astro::discover::InstrumentComponent	camera = instrument->get(
			astro::discover::InstrumentComponentKey::Camera,
				parameters.cameraIndex);
		info.camera(camera.deviceurl());
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found camera %s",
			info.camera().c_str());
	} else {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no camera components");
	}

	if ((instrument->nComponentsOfType(
		astro::discover::InstrumentComponentKey::CCD) > 0)
		&& (parameters.ccdIndex >= 0)) {
		astro::discover::InstrumentComponent	ccd = instrument->get(
			astro::discover::InstrumentComponentKey::CCD,
				parameters.ccdIndex);
		info.ccd(ccd.deviceurl());
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found ccd %s",
			info.ccd().c_str());
	} else {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no CCD components");
	}

	if ((instrument->nComponentsOfType(
		astro::discover::InstrumentComponentKey::Cooler) > 0)
		&& (parameters.coolerIndex >= 0)) {
		astro::discover::InstrumentComponent	cooler = instrument->get(
			astro::discover::InstrumentComponentKey::Cooler,
				parameters.coolerIndex);
		info.cooler(cooler.deviceurl());
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found cooler %s",
			info.cooler().c_str());
	} else {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no cooler components");
	}

	if ((instrument->nComponentsOfType(
		astro::discover::InstrumentComponentKey::FilterWheel) > 0)
		&& (parameters.filterwheelIndex >= 0)) {
		astro::discover::InstrumentComponent	filterwheel = instrument->get(
			astro::discover::InstrumentComponentKey::FilterWheel,
				parameters.filterwheelIndex);
		info.filterwheel(filterwheel.deviceurl());
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found filterwheel %s",
			info.filterwheel().c_str());
	} else {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no filterwheel components");
	}

	if ((instrument->nComponentsOfType(
		astro::discover::InstrumentComponentKey::Mount) > 0)
		&& (parameters.mountIndex >= 0)) {
		astro::discover::InstrumentComponent	mount = instrument->get(
			astro::discover::InstrumentComponentKey::Mount,
				parameters.mountIndex);
		info.mount(mount.deviceurl());
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found mount %s",
			info.mount().c_str());
	} else {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "no mount components");
	}

	try {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "submitting new task");
		int	id = taskqueue.submit(snowstar::convert(parameters), info);
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::TASK,
			astro::stringprintf("task %d submitted", id));
		return id;
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot submit: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw BadParameter(cause);
	}
}

TaskParameters TaskQueueI::parameters(int taskid, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "query parameters of task %d", taskid);
	if (!taskqueue.exists(taskid)) {
		std::string	cause = astro::stringprintf(
			"task %d does not exist", taskid);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw NotFound(cause);
	}
	try {
		return snowstar::convert(taskqueue.parameters(taskid));
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot get parameters for task %d: %s %s", taskid,
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw NotFound(cause);
	}
}

TaskInfo TaskQueueI::info(int taskid, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "query info of task %d", taskid);
	if (!taskqueue.exists(taskid)) {
		std::string	cause = astro::stringprintf(
			"task %d does not exist", taskid);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw NotFound(cause);
	}
	try {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "return task info(%d)", taskid);
		return snowstar::convert(taskqueue.info(taskid));
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot get info for task %d: %s %s", taskid,
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw NotFound(cause);
	}
}

void TaskQueueI::cancel(int taskid, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "cancel request for %d", taskid);
	if (!taskqueue.exists(taskid)) {
		std::string	cause = astro::stringprintf(
			"task %d does not exist", taskid);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw NotFound(cause);
	}
	try {
		taskqueue.cancel(taskid);
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::TASK,
			astro::stringprintf("task %d cancelled", taskid));
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot cancel task %d: %s %s", taskid,
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw BadParameter(cause);
	}
}

void TaskQueueI::remove(int taskid, const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "remove request for %d", taskid);
	if (!taskqueue.exists(taskid)) {
		std::string	cause = astro::stringprintf(
			"task %d does not exist", taskid);
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw NotFound(cause);
	}
	try {
		taskqueue.remove(taskid);
		astro::event(EVENT_CLASS, astro::events::INFO,
			astro::events::Event::TASK,
			astro::stringprintf("task %d removed", taskid));
	} catch (const std::exception& x) {
		std::string	 cause = astro::stringprintf(
			"cannot cancel task %d: %s %s", taskid,
			astro::demangle(typeid(x).name()).c_str(), x.what());
		debug(LOG_ERR, DEBUG_LOG, 0, "%s", cause.c_str());
		throw BadParameter(cause);
	}
}

taskidsequence TaskQueueI::tasklist(TaskState state,
		const Ice::Current& /* current */) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "list tasks in state %s",
		astro::task::TaskInfo::state2string(convert(state)).c_str());
	std::list<long>	taskidlist = taskqueue.tasklist(snowstar::convert(state));
	taskidsequence	result;
	std::copy(taskidlist.begin(), taskidlist.end(), back_inserter(result));
	return result;
}

TaskPrx TaskQueueI::getTask(int taskid, const Ice::Current& current) {
	// make sure the task exists
	if (!taskqueue.exists(taskid)) {
		throw NotFound("task does not exist");
	}

	// create an identity for this task
	std::ostringstream	out;
	out << "task/" << taskid;
	std::string	identity = out.str();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "identity for task: %s",
		identity.c_str());

	// create the proxy
	return createProxy<TaskPrx>(identity, current, false);
}

void	TaskQueueI::registerMonitor(const Ice::Identity& callback,
		const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "register a new monitor callback");
	try {
		callbacks.registerCallback(callback, current);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot register callback: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"cannot register callback for unknown reason");
	}
}

void	TaskQueueI::unregisterMonitor(const Ice::Identity& callback,
		const Ice::Current& current) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "unregistering a monitor callback");
	try {
		callbacks.unregisterCallback(callback, current);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot register callback: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"cannot register callback for unknown reason");
	}
}

void	TaskQueueI::taskUpdate(const astro::callback::CallbackDataPtr data) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "TaskQueueI::taskUpdate called");
	try {
		callbacks(data);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "cannot send callback: %s %s",
			astro::demangle(typeid(x).name()).c_str(), x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"cannot send callback for unknown reason");
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "TaskQueueI::taskUpdate completed");
}

} // namespace snowstar
