#
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_REVISION([1.0])
AC_INIT([astro], [1.0], [andreas.mueller@hsr.ch])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT
AC_CONFIG_SRCDIR([include/AstroImage.h])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_HEADERS(include/config.h)


# some configuration directives

# Checks for programs
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_CHECK_TOOL(FLEX, flex)
AC_CHECK_TOOL(BISON, bison)
AC_CHECK_PROG(PDFLATEX, pdflatex, pdflatex)
if test -z "$PDFLATEX"; then
	AC_MSG_WARN([Unabel to create PDF version of the user manual.])
fi
AM_CONDITIONAL([HAVE_PDFLATEX], test -n "$PDFLATEX")

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h stdio.h unistd.h math.h errno.h string.h stdarg.h libintl.h locale.h syslog.h sys/types.h sys/stat.h sys/time.h dirent.h fcntl.h sys/file.h dlfcn.h libusb-1.0/libusb.h fftw3.h Accelerate/Accelerate.h termios.h sys/mman.h execinfo.h sys/select.h poll.h signal.h sys/param.h getopt.h uuid/uuid.h netdb.h assert.h])

# We want to use backtrace API for stack dumps, this requires the use of
# the -rdynamic option during compile
LDFLAGS="${LDFLAGS} -rdynamic"

# Check for libraries that we cannot do without
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([usb-1.0], [libusb_bulk_transfer])
AC_CHECK_LIB([lapack], [dgels_], [LIBS="-llapack $LIBS"],
	AC_MSG_ERROR([required library LAPACK not found]))
AC_CHECK_LIB([fftw3], [fftw_malloc])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([glpk], [glp_create_prob])
AC_CHECK_LIB([sqlite3], [sqlite3_initialize])

## CFITS #############################################################
# if cfits is installed with pkginfo, we use that
if pkg-config --exists cfitsio
then
	LIBS="${LIBS} `pkg-config --libs cfitsio`"
	CXXFLAGS="${CXXFLAGS} `pkg-config --cflags cfitsio`"
else
AC_CHECK_LIB([cfitsio], [ftffrw_])
fi

## UUID ##############################################################
# if uuid is configured with pkg-config
if pkg-config --exists uuid
then
	LIBS="${LIBS} `pkg-config --libs uuid`"
	CXXFLAGS="${CXXFLAGS} `pkg-config --cflags uuid`"
else
AC_CHECK_LIB([uuid], [uuid_parse])
fi

## LIBXML2 ###########################################################
if pkg-config --exists libxml-2.0
then
	LIBS="${LIBS} `pkg-config --libs libxml-2.0`"
	CXXFLAGS="${CXXFLAGS} `pkg-config --cflags libxml-2.0`"
else
	CXXFLAGS="${CXXFLAGS} -I/usr/include/libxml2"
	AC_LANG_PUSH([C++])
	AC_CHECK_HEADERS(libxml/parser.h, ,
		[AC_MSG_ERROR([libxml2 header not found])])
	AC_LANG_POP([C++])
	AC_CHECK_LIB([xml2], [xmlSAXUserParseFile])
fi

## DNS-DS ############################################################
servicediscovery=
case `uname` in
Linux)	servicediscovery=avahi
	AC_DEFINE([USE_SD_AVAHI], [1], [Define to 1 if using AVAHI for service discovery.])
	# find out whether we have AVAHI support
	if pkg-config --exists avahi-client
	then
		LIBS="${LIBS} `pkg-config --libs avahi-client`"
		CXXFLAGS="${CXXFLAGS} `pkg-config --cflags avahi-client`"
	fi
	;;
Darwin)	servicediscovery=bonjour
	AC_DEFINE([USE_SD_BONJOUR], [1], [Define to 1 if using Bonjour for service discovery.])
	;;
esac
AM_CONDITIONAL(ENABLE_AVAHI, test x$servicediscovery = xavahi)
AM_CONDITIONAL(ENABLE_BONJOUR, test x$servicediscovery = xbonjour)

## ICE ###############################################################
# detect ICE support (www.zeroc.com)
#
# unfortunately, ICE does not have a well developed installation concept
# the default installation goes into /opt/Ice-3.5.1, but on Mac OS X it
# is /Library/Developer/Ice-3.5.1. Furthermore, the binaries on Mac OS X
# are in a subdirectory ICEDIR/bin/c++ instead of ICEDIR/bin.
#

# we start with empty variables, and try to fill them step by step
ICEDIR=
ICELIBS=
ICEINCLUDE=
SLICEDIR=
AC_ARG_WITH([ice],
[AS_HELP_STRING([--with-ice=dir], [build the ICE components from path])],
[
	# given the installation directory, we can derive the path
	ICEDIR=${with_ice}
	PATH=${PATH}:${ICEDIR}/bin:${ICEDIR}/c++/bin

	# get the library directories
	if test -d ${ICEDIR}/lib64
	then
		if test -d ${ICEDIR}/lib64/c++11
		then
			ICELIBS="-L${ICEDIR}/lib64/c++11 -Wl,-rpath,${ICEDIR}/lib64/c++11"
		else
			ICELIBS="-L${ICEDIR}/lib64 -Wl,-rpath,${ICEDIR}/lib64"
		fi
	else
		if test -d ${ICEDIR}/lib
		then
			if test -d ${ICEDIR}/lib/c++11
			then
				ICELIBS="-L${ICEDIR}/lib/c++11 -Wl,-rpath,${ICEDIR}/lib/c++11"
			else
				ICELIBS="-L${ICEDIR}/lib -Wl,-rpath,${ICEDIR}/lib"
			fi
		fi
	fi

	# get the include directory, if we cannot find it below the ICE
	# directory, we will assume that it is already on the include path
	if test -d ${ICEDIR}/include
	then
		ICEINCLUDE=-I${ICEDIR}/include
	fi

	# get the slice directory. However, there are cases where the slice
	# files are in a completely different location (like in Ubunt),
	# so we will probably need additional configure options to point
	if test -d ${ICEDIR}/slice
	then
		SLICEDIR=${ICEDIR}/slice
	fi
],[
])
ICELIBS="${ICELIBS} -lIce -lIceUtil"
AC_SUBST(ICEDIR)
AC_SUBST(ICELIBS)
AC_SUBST(ICEINCLUDE)

# set the slice directory, and throwing an error if don't find it
AC_ARG_WITH([slice],
[AS_HELP_STRING([--with-slice=dir], [where to find the slice definitions])],
[
	SLICEDIR=${with_slice}
],[
	if test -z "${SLICEDIR}"
	then
		AC_MSG_ERROR([slice directory not found])
	fi
])
AC_SUBST(SLICEDIR)

# with the path including the slice directory, the slice2cpp command
# should be found now
AC_PATH_TOOL(SLICE2CPP, slice2cpp)

case `uname` in
Linux)	CFLAGS="${CFLAGS} -fopenmp"
	CXXFLAGS="${CXXFLAGS} -fopenmp"
	;;
esac

## UNITTESTS #########################################################
# decide whether or not to build the test programs
AC_ARG_ENABLE(unittests,
[AS_HELP_STRING([--enable-unittests], [also build the unit tests if CPPUNIT is present])],
[
],[
])
if pkg-config --exists cppunit
then
	if test x$enable_unittests = xyes
	then
		CXXFLAGS="${CXXFLAGS} `pkg-config --cflags cppunit`"
		LIBS="${LIBS} `pkg-config --libs cppunit`"
	fi
else
	enable_unittests=no
fi
AM_CONDITIONAL(ENABLE_UNITTESTS, test x$enable_unittests = xyes)

## UVC ###############################################################
# decide whether to build the UVC driver, by default we only build
# it on Mac OS X
AC_ARG_ENABLE(uvc,
[AS_HELP_STRING([--enable-uvc], [build the UVC driver])],
[
],[
	case `uname` in
	Darwin)	enable_uvc=yes
		;;
	*)	enable_uvc=no
		;;
	esac
])
AM_CONDITIONAL(ENABLE_UVC, test x$enable_uvc = xyes)

## SX ################################################################
# decide whether to build the SX driver
AC_ARG_ENABLE(sx,
[AS_HELP_STRING([--enable-sx],[build the SX driver])],
[
	enable_sx=yes
],[
	enable_sx=no
])
AM_CONDITIONAL(ENABLE_SX, test x$enable_sx = xyes)

## OTHELLO ###########################################################
# decide whether to build the OTHELLO driver
AC_ARG_ENABLE(othello,
[AS_HELP_STRING([--enable-othello],[build the Othello driver])],
[
	enable_othello=yes
],[
	enable_othello=no
])
AM_CONDITIONAL(ENABLE_OTHELLO, test x$enable_othello = xyes)

## ASI ###############################################################
# check for the ASI library
ASIINCLUDE=
ASILIBPATH=
ASILIBS="-lASICamera2"
AC_ARG_WITH(asi,
[AS_HELP_STRING([--with-asi=dir], [location of ASI header/library])],
[
	ASIINCLUDE="-I${with_asi}/include"
	ASILIBPATH="-L${with_asi}/lib -L${with_asi}/lib64"
],
)
AC_ARG_ENABLE(asi,
[AS_HELP_STRING([--enable-asi],[build the ASI driver])],
[
	enable_asi=yes
],[
	enable_asi=no
])
OLDINCLUDE=${CPPFLAGS}
if test .${enable_asi} = .yes
then
	CPPFLAGS="${CPPFLAGS} ${ASIINCLUDE}"
	AC_CHECK_HEADERS(ASICamera2.h, enable_asi=yes, enable_asi=no)
fi
if test .${enable_asi} = .yes
then
	OLDLDFLAGS="${LDFLAGS}"
	OLDLIBS="${LIBS}"
	LDFLAGS="${LDFLAGS} ${ASILIBPATH}"
	LIBS="${LIBS} ${ASILIBS}"
	AC_CHECK_LIB(ASICamera2, ASIGetNumOfConnectedCameras)
	LDFLAGS="${OLDLDFLAGS}"
	LIBS="${OLDLIBS}"
else
	CPPFLAGS="${OLDINCLUDE}"
fi
AC_SUBST(ASIINCLUDE)
AC_SUBST(ASILIBPATH)
AC_SUBST(ASILIBS)
AM_CONDITIONAL(ENABLE_ASI, test x$enable_asi = xyes)

## ATIK ##############################################################
# checks for ATIK libary, currently only dummy settings
ATIKINCLUDEE=
ATIKLIBPATH=
ATIKLIBS=
AC_ARG_WITH(atik,
[AS_HELP_STRING([--with-atik=dir], [location of ATIK header/library])],
[
	ATIKINCLUDE="-I${with_atik}/include"
	ATIKLIBPATH="-L${with_atik}/lib -L${with_atik}/lib64"
],
)
AC_ARG_ENABLE(atik,
[AS_HELP_STRING([--enable-atik],[build the ATIK driver])],
[
	enable_atik=yes
],[
	enable_atik=no
])
OLDINCLUDE=${CPPFLAGS}
if test .${enable_atik} = .yes
then
	CPPFLAGS="${CPPFLAGS} ${ATIKINCLUDE}"
	AC_LANG_PUSH([C++])
	AC_CHECK_HEADERS(atikccdusb.h, enable_atik=yes, enable_atik=no)
	AC_LANG_POP([C++])
fi
if test .${enable_atik} = .yes
then
	OLDLDFLAGS="${LDFLAGS}"
	OLDLIBS="${LIBS}"
	LDFLAGS="${LDFLAGS} ${ATIKLIBPATH}"
	LIBS="${LIBS} ${ATIKLIBS}"
	AC_LANG_PUSH([C++])
	AC_CHECK_LIB(atikccd, AtikCamera_list)
	ATIKLIBS=-latikccd
	AC_LANG_POP([C++])
	LDFLAGS="${OLDLDFLAGS}"
	LIBS="${OLDLIBS}"
else
	CPPFLAGS="${OLDINCLUDE}"
fi
AC_SUBST(ATIKINCLUDE)
AC_SUBST(ATIKLIBPATH)
AC_SUBST(ATIKLIBS)
AM_CONDITIONAL(ENABLE_ATIK, test x$enable_atik = xyes)

## SBIG ##############################################################
# check for SBIG library
AC_ARG_ENABLE(sbig,
[AS_HELP_STRING([--enable-sbig], [build the proprietary SBIG driver])],
[
	case `uname` in
	Darwin)	SBIGTARGET=ENV_MACOSX
		# The driver installed by the SBIG installation driver
		# no longer works out of the box, because apparently
		# /Library/Frameworks is no longer on the system framework
		# search path
		SBIGINCLUDE="-F /Library/Frameworks"
		;;
	Linux)	SBIGTARGET=ENV_LINUX
		SBIGINCLUDE=-I/usr/local/include
		;;
	esac
	AC_SUBST(SBIGTARGET)
	OLDCFLAGS="${CFLAGS}"
	CFLAGS="${CFLAGS} -DTARGET=${SBIGTARGET} ${SBIGINCLUDE}"
	case `uname` in
	Linux)	OLDLIBS="${LIBS}"
		AC_CHECK_HEADERS(lpardrv.h, ,
			[AC_MSG_ERROR([SBIG header not found])])
		AC_CHECK_LIB([sbigudrv], [SBIGUnivDrvCommand], enable_sbig=yes,
			[AC_MSG_ERROR([SBIG library not found])])
		SBIGLIBRARY=-lsbigudrv
		SBIGLDFLAGS=
		LIBS="${OLDLIBS}"
		;;
	Darwin)	
		AC_CHECK_HEADERS(SBIGUDrv/lpardrv.h, , [AC_MSG_ERROR([SBIG header not found])])
		echo "Mac OS X: header was found, so library will work"
		SBIGLDFLAGS="-F/Library/Frameworks"
		SBIGLIBRARY="-framework SBIGUDrv"
		CFLAGS="${CFLAGS} ${SBIGLIBRARY}"
		;;
	esac
	AC_SUBST(SBIGLDFLAGS)
	AC_SUBST(SBIGLIBRARY)
	AC_SUBST(SBIGINCLUDE)
	CFLAGS="${OLDCFLAGS}"
],[
enable_sbig=no
]
)
AM_CONDITIONAL(ENABLE_SBIG, test x$enable_sbig = xyes)

## QSI ###############################################################
# check for qsi library
AC_ARG_ENABLE(qsi,
[AS_HELP_STRING([--enable-qsi], [build the QSI driver])],
[
	AC_CHECK_PROGS(LIBQSIAPICONFIG, libqsiapi-config, /bin/false)
	if ${LIBQSIAPICONFIG} --version > /dev/null
	then
		:
	else
		AC_MSG_ERROR([QSI configuration utility not found])
	fi
	# don't enable the QSI driver unless you find the header
	# and the library
	enable_qsi=no

	# check whether the qsiapi.h header is present
	AC_LANG_PUSH([C++])
	AC_CHECK_HEADERS([qsiapi.h])
	AC_CHECK_HEADER([qsiapi.h],
		[AC_DEFINE([HAVE_QSIAPI_H], [1],
		[Define to 1 if you have <qsiapi.h>.])],
		[AC_MSG_ERROR([QSI header not available, install or disable QSI driver])])
	AC_LANG_POP([C++])

	QSICFLAGS="`${LIBQSIAPICONFIG} --cflags`"
	QSILIBS="`${LIBQSIAPICONFIG} --libs`"
	# if we fail, disable qsi
	enable_qsi=yes
],[
	enable_qsi=no
	QSICFLAGS=
	QSILIBS=
]
)
AM_CONDITIONAL(ENABLE_QSI, test x$enable_qsi = xyes)
AC_SUBST(QSICFLAGS)
AC_SUBST(QSILIBS)

## QHY ###############################################################
# enable the qhy driver
AC_ARG_ENABLE(qhy,
[AS_HELP_STRING([--enable-qhy], [build the QHY driver])],
[
	enable_qhy=yes
],[
	enable_qhy=no
])
# try various possible locations for the QHY library
tryqhylibs () {
	if test -r /usr/local/lib64/libqhyccd.la
	then
		QHYLIB=/usr/local/lib64/libqhyccd.la
		return
	fi
	if test -r /usr/local/lib/libqhyccd.la
	then
		QHYLIB=/usr/local/lib/libqhyccd.la
		return
	fi
	if test -r /usr/lib64/libqhyccd.la
	then
		QHYLIB=/usr/lib64/libqhyccd.la
		return
	fi
	if test -r /usr/lib/libqhyccd.la
	then
		QHYLIB=/usr/lib/libqhyccd.la
		return
	fi
}
if test ${enable_qhy} = yes
then
	tryqhylibs
fi
AC_SUBST(QHYLIB)
AM_CONDITIONAL(ENABLE_QHY, test x$enable_qhy = xyes)

## UNICAP ############################################################
# check for the Unicap library
AC_ARG_ENABLE(unicap,
[AS_HELP_STRING([--enable-unicap], [build the unicap driver])],
[
	if test x${enable_unicap} = xyes
	then
		AC_CHECK_HEADERS(unicap.h, ,
			[AC_MSG_ERROR([unicap header not found])])
		AC_CHECK_LIB([unicap], [unicap_open], LIBS="${LIBS} -lunicap",
			[AC_MSG_ERROR([unicap library not found])])
	fi
],[
]
)
AM_CONDITIONAL(ENABLE_UNICAP, test x${enable_unicap} = xyes)

## MICROTOUCH ########################################################
# turn on the microtouch stuff
AC_ARG_ENABLE(microtouch,
[AS_HELP_STRING([--enable-microtouch], [build the microtouch driver])], ,
	enable_microtouch=yes)
AM_CONDITIONAL(ENABLE_MICROTOUCH, test x${enable_microtouch} = xyes)


######################################################################

# Checks for typedefs
AC_C_CONST

# Checks for library functions
AC_CHECK_FUNCS([memset strdup strerror trunc])

# device property file location
DEVICEPROPERTIES=${sysconfdir}/device.properties
AC_SUBST(DEVICEPROPERTIES)

# default pid file location
PIDDIR=${localstatedir}/run
AC_SUBST(PIDDIR)

# definitions for the extension for shared libraries
case `uname` in
	Darwin*)
		SHLIB_EXT=dylib
		;;
	*)
		SHLIB_EXT=so
		;;
esac
AC_SUBST(SHLIB_EXT)

AC_CONFIG_FILES([Makefile include/Makefile
	lib/Makefile
	lib/utils/Makefile lib/utils/test/Makefile
	lib/persistence/Makefile lib/persistence/test/Makefile
	lib/discovery/Makefile lib/discovery/test/Makefile
	lib/event/Makefile lib/event/test/Makefile
	lib/image/Makefile lib/image/test/Makefile
	lib/processing/Makefile lib/processing/test/Makefile
	lib/device/Makefile lib/device/test/Makefile
	lib/config/Makefile lib/config/test/Makefile
	lib/catalogs/Makefile lib/catalogs/test/Makefile
	lib/task/Makefile lib/task/test/Makefile
	lib/guiding/Makefile lib/guiding/test/Makefile
	lib/focusing/Makefile lib/focusing/test/Makefile
	lib/usb/Makefile lib/usb/test/Makefile
	lib/unicap/Makefile lib/unicap/test/Makefile
	lib/test/Makefile
	ice/Makefile ice/slice/Makefile ice/include/Makefile
	ice/lib/Makefile ice/lib/common/Makefile ice/lib/client/Makefile
	ice/lib/conversion/Makefile ice/lib/server/Makefile
	ice/client/Makefile ice/client/guide/Makefile
	ice/server/Makefile
	tools/Makefile tools/config/Makefile tools/mount/Makefile
	tools/devices/Makefile tools/image/Makefile tools/guiding/Makefile
	tools/catalog/Makefile tools/process/Makefile tools/process/process.xml
	drivers/Makefile drivers/mock1/Makefile drivers/mock2/Makefile
	drivers/simulator/Makefile drivers/net/Makefile
	drivers/sx/Makefile drivers/sbig/Makefile drivers/othello/Makefile
	drivers/uvc/Makefile drivers/unicap/Makefile 
	drivers/microtouch/Makefile drivers/qsi/Makefile drivers/qhy/Makefile
	drivers/celestron/Makefile drivers/gpio/Makefile drivers/nice/Makefile
	drivers/asi/Makefile drivers/atik/Makefile
	man/Makefile doc/Makefile doc/manual/Makefile
	scripts/Makefile scripts/startastro
	test/Makefile])
AC_OUTPUT

