/*
 * snowtask.cpp -- submit a task or monitor the execution of tasks on a server
 *
 * (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
 */
#include <AstroUtils.h>
#include <AstroDebug.h>
#include <CommunicatorSingleton.h>
#include <includes.h>
#include <AstroConfig.h>
#include <tasks.h>
#include <IceConversions.h>
#include <CommonClientTasks.h>
#include <AstroFormat.h>

namespace snowstar {
namespace app {
namespace snowtask {

astro::ServerName	servername;
bool	completed = false;
bool	verbose = false;
bool	dryrun = false;
astro::camera::Exposure	exposure;
std::string	instrumentname;
int	filterposition;
double	temperature = 273.15;

void	signal_handler(int /* sig */) {
	completed = true;
}

/**
 * \brief Convert task state to a string for display
 */
static std::string	state2string(TaskState state) {
	switch (state) {
	case TskPENDING:
		return std::string("pending");
		break;
	case TskEXECUTING:
		return std::string("executing");
		break;
	case TskFAILED:
		return std::string("failed");
		break;
	case TskCANCELLED:
		return std::string("cancelled");
		break;
	case TskCOMPLETED:
		return std::string("completed");
		break;
	}
	throw std::runtime_error("unknown task state code");
}

static TaskState	string2taskstate(const std::string& statestring) {
	if (statestring == "pending") {
		return TskPENDING;
	}
	if (statestring == "executing") {
		return TskEXECUTING;
	}
	if (statestring == "failed") {
		return TskFAILED;
	}
	if (statestring == "cancelled") {
		return TskCANCELLED;
	}
	if (statestring == "completed") {
		return TskCOMPLETED;
	}
	throw std::runtime_error("unknown task state name");
}

/**
 * \brief Convert queue state to a string for display
 */
static std::string	state2string(QueueState state) {
	switch (state) {
	case QueueIDLE:
		return std::string("idle");
	case QueueLAUNCHING:
		return std::string("launching");
	case QueueSTOPPING:
		return std::string("stopping");
	case QueueSTOPPED:
		return std::string("stopped");
	}
	throw std::runtime_error("unknown queue state code");
}

/**
 * \brief A monitor implementation todisplay state changes
 */
class TaskMonitorI : public TaskMonitor {
public:
	TaskMonitorI() {
		std::cout << "Date       Time     Id     new state";
		std::cout << std::endl;
	}
	void	stop(const Ice::Current& /* current */) {	
		completed = true;
	}
	void	update(const TaskMonitorInfo& info,
		const Ice::Current& /* current */) {
		time_t	t = converttime(info.timeago);
		std::cout << astro::timeformat("%Y-%m-%d %H:%M:%S", t);
		std::cout << astro::stringprintf(" %6d %s", info.taskid,
			state2string(info.newstate).c_str());
		std::cout << std::endl;
	}
};

/**
 * \brief Implementation of the monitor command
 */
int	command_monitor(TaskQueuePrx tasks) {
	// create a monitor callback
	Ice::ObjectPtr	callback = new TaskMonitorI();

	// register the callback with the server
	Ice::CommunicatorPtr	ic = CommunicatorSingleton::get();
	CallbackAdapter	adapter(ic);
	Ice::Identity	ident = adapter.add(callback);
	tasks->ice_getConnection()->setAdapter(adapter.adapter());
	tasks->registerMonitor(ident);

	// wait for the termination
	signal(SIGINT, signal_handler);
	while (!completed) {
		sleep(1);
	}

	// unregister callback before exiting
	tasks->unregisterMonitor(ident);

	// that's it
	return EXIT_SUCCESS;
}

std::string	when(double timeago) {
	time_t	t = converttime(timeago);
	//debug(LOG_DEBUG, DEBUG_LOG, 0, "timeago = %.0f, %24.24s", timeago,
	//	ctime(&t));
	struct tm	*tp = localtime(&t);
	char	buffer[128];
	if (timeago > 86400 * 365) {
		strftime(buffer, sizeof(buffer), "%y %b", tp);
	}
	if (timeago > 86400) {
		strftime(buffer, sizeof(buffer), "%b %d", tp);
	}
	if (timeago <= 86400) {
		strftime(buffer, sizeof(buffer), "%H:%M:%S", tp);
	}
	return std::string(buffer);
}

/**
 * \brief Implementation of the list command
 */
int	common_list(TaskQueuePrx tasks, const std::set<int> ids) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "listing %d tasks", ids.size());
	std::set<int>::const_iterator	i;
	std::cout << "task S size      bin time  temp when     ";
	if (verbose) {
		std::cout << astro::stringprintf("%-40.40s", "camera");
	}
	std::cout << "info" << std::endl;
	for (auto ptr = ids.begin(); ptr != ids.end(); ptr++) {
		TaskParameters	parameters = tasks->parameters(*ptr);
		TaskInfo	info = tasks->info(*ptr);
		std::cout << astro::stringprintf("%4d ", info.taskid);
		switch (info.state) {
		case TskPENDING:
			std::cout << "P";
			break;
		case TskEXECUTING:
			std::cout << "E";
			break;
		case TskFAILED:
			std::cout << "F";
			break;
		case TskCANCELLED:
			std::cout << "X";
			break;
		case TskCOMPLETED:
			std::cout << "C";
			break;
		}
		std::string	s = astro::stringprintf("%dx%d",
				info.frame.size.width, info.frame.size.height);
		std::cout << astro::stringprintf(" %-9.9s %1dx%1d %4.0f %5.1f",
			s.c_str(),
			parameters.exp.mode.x, parameters.exp.mode.y,
			parameters.exp.exposuretime,
			parameters.ccdtemperature - 273.15);
		std::cout << astro::stringprintf(" %-8.8s ",
			when(info.lastchange).c_str());
		if (verbose) {
			std::cout << astro::stringprintf("%-40.40s",
				parameters.camera.c_str());
		}
		switch (info.state) {
		case TskPENDING:
		case TskEXECUTING:
			break;
		case TskFAILED:
		case TskCANCELLED:
			std::cout << info.cause;
			break;
		case TskCOMPLETED:
			std::cout << info.filename;
			break;
		}
		std::cout << std::endl;
	}
	return EXIT_SUCCESS;
}

/**
 * \brief Implementation of the list command
 */
int	command_list(TaskQueuePrx tasks, const std::string& statestring) {
	TaskState	state = string2taskstate(statestring);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "looking for %s tasks",
		statestring.c_str());

	// request all the task ids of this type
	std::set<int>	ids;
	taskidsequence	tasksequence = tasks->tasklist(state);
	std::copy(tasksequence.begin(), tasksequence.end(),
		std::inserter(ids, ids.begin()));
	debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d tasks of state %s",
		ids.size(), statestring.c_str());

	// list all tasks in the set
	return common_list(tasks, ids);
}

int	command_list(TaskQueuePrx tasks) {
	debug(LOG_DEBUG, DEBUG_LOG, 0, "looking for tasks of all states");
	std::set<int>	ids;
	{
		taskidsequence	result = tasks->tasklist(TskPENDING);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d pending tasks",
			result.size());
		std::copy(result.begin(), result.end(),
			std::inserter(ids, ids.begin()));
	}
	{
		taskidsequence	result = tasks->tasklist(TskEXECUTING);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d executing tasks",
			result.size());
		std::copy(result.begin(), result.end(),
			std::inserter(ids, ids.begin()));
	}
	{
		taskidsequence	result = tasks->tasklist(TskFAILED);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d failed tasks",
			result.size());
		std::copy(result.begin(), result.end(),
			std::inserter(ids, ids.begin()));
	}
	{
		taskidsequence	result = tasks->tasklist(TskCANCELLED);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d cancelled tasks",
			result.size());
		std::copy(result.begin(), result.end(),
			std::inserter(ids, ids.begin()));
	}
	{
		taskidsequence	result = tasks->tasklist(TskCOMPLETED);
		debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d completed tasks",
			result.size());
		std::copy(result.begin(), result.end(),
			std::inserter(ids, ids.begin()));
	}
	debug(LOG_DEBUG, DEBUG_LOG, 0, "found %d ids total", ids.size());
	return common_list(tasks, ids);
}

/**
 * \brief Implementation of the start command
 */
int	command_start(TaskQueuePrx tasks) {
	tasks->start();
	return EXIT_SUCCESS;
}

/**
 * \brief Implementation of the stop command
 */
int	command_stop(TaskQueuePrx tasks) {
	tasks->stop();
	return EXIT_SUCCESS;
}

/**
 * \brief Implementation of the state command
 */
int	command_state(TaskQueuePrx tasks) {
	QueueState	queuestate = tasks->state();
	std::cout << state2string(queuestate) << std::endl;
	return EXIT_SUCCESS;
}

class TaskRemover {
	TaskQueuePrx&	_tasks;
public:
	TaskRemover(TaskQueuePrx& tasks) : _tasks(tasks) { }
	void	operator()(int id);
};

void	TaskRemover::operator()(int id) {
	if (dryrun) {
		std::cout << "task " << id;
		std::cout << " not removed (dry run)";
		std::cout << std::endl;
	} else {
		try {
			_tasks->remove(id);
		} catch (std::exception& x) {
			std::cerr << "cannot remove task " << id << ": ";
			std::cerr << x.what() << std::endl;
		}
	}
}

class TaskCanceller {
	TaskQueuePrx&	_tasks;
public:
	TaskCanceller(TaskQueuePrx& tasks) : _tasks(tasks) { }
	void	operator()(int id);
};

void	TaskCanceller::operator()(int id) {
	if (dryrun) {
		std::cout << "task " << id;
		std::cout << " not cancelled (dry run)";
		std::cout << std::endl;
	} else {
		try {
			_tasks->cancel(id);
		} catch (std::exception& x) {
			std::cerr << "cannot cancel task " << id << ": ";
			std::cerr << x.what() << std::endl;
		}
	}
}

/**
 * \brief Implementation of the remove command
 */
int	command_remove(TaskQueuePrx tasks, std::list<int> ids) {
	std::for_each(ids.begin(), ids.end(), TaskRemover(tasks));
	return EXIT_SUCCESS;
}

int	command_cancel(TaskQueuePrx tasks, std::list<int> ids) {
	std::for_each(ids.begin(), ids.end(), TaskCanceller(tasks));
	return EXIT_SUCCESS;
}

int	command_submit(TaskQueuePrx tasks) {
	TaskParameters	parameters;
	parameters.camera = "camera:simulator/camera";
	parameters.ccdid = 0;
	parameters.ccdtemperature = temperature;
	parameters.filterwheel = "";
	parameters.filterposition = filterposition;
	parameters.exp = convert(exposure);
	int	taskid = tasks->submit(parameters);
	debug(LOG_DEBUG, DEBUG_LOG, 0, "submitted new task %d", taskid);
	return EXIT_FAILURE;
}

int	command_image(TaskQueuePrx tasks, int id, const std::string& filename) {
	// check whether the task really is completed
	TaskInfo	info = tasks->info(id);
	if (TskCOMPLETED != info.state) {
		throw std::runtime_error("task not completed");
	}

	// get an interfaces for Images
	Ice::CommunicatorPtr	ic = CommunicatorSingleton::get();
        Ice::ObjectPrx  base = ic->stringToProxy(servername.connect("Images"));
	ImagesPrx	images = ImagesPrx::checkedCast(base);

	// get an interface for that particular image
	ImagePrx	image = images->getImage(info.filename);
	ImageFile	imagefile = image->file();

	// write the image data into a file
	int fd = open(filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (fd < 0) {
		throw std::runtime_error("cannot create file");
	}
	if (imagefile.size() != write(fd, imagefile.data(), imagefile.size())) {
		std::string	msg = astro::stringprintf("cannot write data: %s",
			strerror(errno));
		debug(LOG_DEBUG, DEBUG_LOG, 0, "cannot write data");
		close(fd);
		throw std::runtime_error(msg);
	}
	close(fd);

	throw std::runtime_error("image command not implemented");
	return EXIT_FAILURE;
}

/**
 * \brief Usage function for the snowtask program
 */
void	usage(const char *progname) {
	astro::Path	path(progname);
	std::string	p = std::string("    ") + path.basename();
	std::cout << "usage:" << std::endl;
	std::cout << std::endl;
	std::cout << p << " [ options ] monitor" << std::endl;
	std::cout << p << " [ options ] list [ state ]" << std::endl;
	std::cout << p << " [ options ] start" << std::endl;
	std::cout << p << " [ options ] stop" << std::endl;
	std::cout << p << " [ options ] state" << std::endl;
	std::cout << p << " [ options ] cancel id ..." << std::endl;
	std::cout << p << " [ options ] remove id ..." << std::endl;
	std::cout << p << " [ options ] submit" << std::endl;
	std::cout << p << " [ options ] image id filename" << std::endl;
	std::cout << std::endl;
	std::cout << "possible task states:" << std::endl;
	std::cout << "    pending    " << std::endl;
	std::cout << "    executing  " << std::endl;
	std::cout << "    failed     " << std::endl;
	std::cout << "    cancelled  " << std::endl;
	std::cout << "    completed  " << std::endl;
	std::cout << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << " -b,--binning=XxY   select XxY binning (default 1x1)"
		<< std::endl;
	std::cout << " -c,--config=<cfg>  use configuration from a cfg"
		<< std::endl;
	std::cout << " -d,--debug         increase debug level" << std::endl;
	std::cout << " -n,--dryrun        suppress actions that would change "
		"the queue" << std::endl;
	std::cout << " -s,--server<srv>   connect to the queue on <srv>"
		<< std::endl;
	std::cout << " -h,--help          show this help and exit" << std::endl;
}

/**
 * \brief Options for the snowtask program
 */
static struct option	longopts[] = {
{ "binning",	required_argument,	NULL,		'b' }, /* 0 */
{ "config",	required_argument,	NULL,		'c' }, /* 1 */
{ "debug",	no_argument,		NULL,		'd' }, /* 2 */
{ "dryrun",	no_argument,		NULL,		'n' }, /* 3 */
{ "exposure",	required_argument,	NULL,		'e' }, /* 4 */
{ "filter",	required_argument,	NULL,		'f' }, /* 5 */
{ "help",	no_argument,		NULL,		'h' }, /* 6 */
{ "instrument",	required_argument,	NULL,		'i' }, /* 7 */
{ "purpose",	required_argument,	NULL,		'p' }, /* 8 */
{ "rectangle",	required_argument,	NULL,		'r' }, /* 9 */
{ "server",	required_argument,	NULL,		's' }, /* 9 */
{ "temperature",required_argument,	NULL,		't' }, /* 0 */
{ "verbose",	no_argument,		NULL,		'v' }, /* 0 */
{ NULL,		0,			NULL,		0   }
};

/**
 * \brief Main function for the snowtask program
 */
int	main(int argc, char *argv[]) {
	CommunicatorSingleton	cs(argc, argv);
	Ice::CommunicatorPtr	ic = CommunicatorSingleton::get();

	// parse command line options
	int	c;
	int	longindex;
	while (EOF != (c = getopt_long(argc, argv, "c:dh?s:v", longopts,
		&longindex)))
		switch (c) {
		case 'b':
			exposure.mode = astro::camera::Binning(optarg);
			break;
		case 'c':
			astro::config::Configuration::set_default(optarg);
			break;
		case 'd':
			debuglevel = LOG_DEBUG;
			break;
		case 'e':
			exposure.exposuretime = std::stod(optarg);
			break;
		case 'f':
			filterposition = std::stod(optarg);
			break;
		case 'h':
		case '?':
			usage(argv[0]);
			return EXIT_SUCCESS;
		case 'i':
			instrumentname = optarg;
			break;
		case 'n':
			dryrun = true;
			break;
		case 'p':
			exposure.purpose = astro::camera::Exposure::string2purpose(optarg);
			break;
		case 'r':
			exposure.frame = astro::image::ImageRectangle(optarg);
			break;
		case 's':
			servername = astro::ServerName(optarg);
			break;
		case 't':
			temperature = 273.15 + std::stod(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		}

	// get the command name
	if (argc <= optind) {
		throw std::runtime_error("command name missing");
	}
	std::string	command = argv[optind++];
	debug(LOG_DEBUG, DEBUG_LOG, 0, "working on command %s",
		command.c_str());

	// get the Tasks interface
        Ice::ObjectPrx  base = ic->stringToProxy(servername.connect("Tasks"));
	TaskQueuePrx	tasks = TaskQueuePrx::checkedCast(base);

	// the monitor command does not need any other 
	if (command == "monitor") {
		return command_monitor(tasks);
	}
	if (command == "start") {
		return command_start(tasks);
	}
	if (command == "stop") {
		return command_stop(tasks);
	}
	if (command == "state") {
		return command_state(tasks);
	}
	if (command == "list") {
		if (optind >= argc) {
			return command_list(tasks);
		}
		std::string	statestring = argv[optind++];
		return command_list(tasks, statestring);
	}
	if (command == "remove") {
		std::list<int>	ids;
		while (optind < argc) {
			ids.push_back(atoi(argv[optind++]));
		}
		return command_remove(tasks, ids);
	}
	if (command == "cancel") {
		std::list<int>	ids;
		while (optind < argc) {
			ids.push_back(atoi(argv[optind++]));
		}
		return command_cancel(tasks, ids);
	}
	if (command == "submit") {
		return command_submit(tasks);
	}
	if (command == "image") {
		if (argc <= optind) {
			throw std::runtime_error("no id argument specified");
		}
		int	id = std::stoi(argv[optind++]);
		if (argc <= optind) {
			throw std::runtime_error("no image file name");
		}
		std::string	filename = argv[optind++];
		return command_image(tasks, id, filename);
	}

	std::cerr << "unknown command: " << command << std::endl;
	return EXIT_FAILURE;
}

} // namespace snowtask
} // namespace app
} // namespace snowtar

int	main(int argc, char *argv[]) {
	return astro::main_function<snowstar::app::snowtask::main>(argc, argv);
}