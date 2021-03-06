/*
 * TaskExecutor.cpp -- queue of tasks
 *
 * (c) 2013 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroTask.h>
#include <AstroDebug.h>
#include <AstroFormat.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <AstroLoader.h>
#include <AstroDevaccess.h>
#include <ImageDirectory.h>
#include <errno.h>
#include <string.h>
#include <AstroIO.h>
#include <ExposureWork.h>

using namespace astro::persistence;
using namespace astro::io;

namespace astro {
namespace task {

/**
 * \brief Springboard function to start the main method of the Executor class
 *
 * Threads can only start functions, but we want to execute a method
 * of a class. So the function we call when constructing the thread
 * has the class as an argument and call the method using this class
 * pointer from this function.
 */
static void	taskmain(TaskExecutor *te) {
	try {
		te->main();
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "task->main() terminated by "
			"%s: %s", demangle(typeid(x).name()).c_str(), x.what());
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0, "taskmain terminated by "
			"unknown exception");
	}
}

/**
 * \brief Task Executor main function
 *
 * Special consideration is needed for cancelling such a thread. If the
 * exposure has already begun, cancelling must extend to the exposure
 * that must be cancelled.
 */
void	TaskExecutor::main() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "TaskExecutor::main() started");
	debug(LOG_DEBUG, DEBUG_LOG, 0, "LOCK(TaskExecutor::release_mutex,%d)",
		_task.id());
	// we try to lock the release_mutex, but because that mutex is
	// locked by the constructor we will normally stop at this point.
	// only when the lock is released by calling the release method
	// will it be possible to acquire the lock and continue.
	release_mutex.lock();

	try {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "entering main task region");
		// inform queue of state change
		_task.state(TaskQueueEntry::executing);
		_queue.post(_task.id()); // notify queue of state change
		debug(LOG_DEBUG, DEBUG_LOG, 0, "update posted");

		// the exposure task starts to run when we call the run method
		// this is also the moment when the 
		exposurework->run();
		_task.state(TaskQueueEntry::complete);
	} catch (CancelException& x) {
		debug(LOG_DEBUG, DEBUG_LOG, 0, "execution cancelled: %s",
			x.what());
		_task.state(TaskQueueEntry::cancelled);
	} catch (const std::exception& x) {
		debug(LOG_ERR, DEBUG_LOG, 0, "executor failure: %s", x.what());
		_task.state(TaskQueueEntry::failed);
	} catch (...) {
		debug(LOG_ERR, DEBUG_LOG, 0,
			"executor failure, unknown exception");
		_task.state(TaskQueueEntry::failed);
	}

	// post the current state to the queue
	_queue.post(_task.id());

	debug(LOG_DEBUG, DEBUG_LOG, 0, "main terminated");
}

/**
 * \brief create a task executor
 *
 * Launch a new thread to work an a given task. When a task starts, we have
 * to wait in the constructor until the new task is up and running and has
 * changed the state from PENDING to anything else. If we didn't do this,
 * then the task queue might launch the task again.
 *
 * \param queue		the queue that ons this executor
 * \param task		the task queue entry describing the task
 */
TaskExecutor::TaskExecutor(TaskQueue& queue, const TaskQueueEntry& task)
	: _queue(queue), _task(task) {
	debug(LOG_DEBUG, DEBUG_LOG, 0,
		"constructor LOCK(TaskExecutor::release_mutex,%d)", _task.id());
	release_mutex.lock();

	// create a new ExposureTask object. The ExposureTask contains
	// the logic to actually execute the task
	exposurework = new ExposureWork(_task);

	// initialize the thread. The constructor will return when the
	// thread has started running. But it will then only proceed to
	// the point where it tries to acquire the release_mutex. Only when
	// we release the lock on the mutex in the release method the
	// thread can go on and post state changes.
	_thread = std::thread(taskmain, this);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "thread launched");
}

/**
 * \brief Release the thread 
 *
 * Because the release_mutex is initially locked by the constructor, the
 * newly launched thread cannot get very far. If the thread constructing
 * the executor is read to receive updates from the thread, it should release
 * the thread using this method.
 */
void	TaskExecutor::release() {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "UNLOCK(TaskExecutor::release_lock,%d)",
		_task.id());
	release_mutex.unlock();
}

/**
 * \brief destroy a task exectuor
 */
TaskExecutor::~TaskExecutor() {
	cancel();
	wait();
	// when we have waited for the thread, we are sure the exposurework
	// is no longer needed, and can be detroyed
	delete exposurework;
	exposurework = NULL;
}

/**
 * \brief cancel execution 
 */
void	TaskExecutor::cancel() {
	exposurework->cancel();
	debug(LOG_DEBUG, DEBUG_LOG, 0, "thread cancel signal sent");
}

/**
 * \brief wait for the thread to terminate
 */
void	TaskExecutor::wait() {
	try {
		_thread.join();
	} catch (...) {
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "thread terminated");
}

/**
 * \brief check whether this executor blocks a given task queue entry
 */
bool	TaskExecutor::blocks(const TaskQueueEntry& other) {
	return _task.blocks(other);
}

} // namespace task
} // namespace astro
