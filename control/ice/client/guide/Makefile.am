#
# Makefile.am -- build ICE test client for the Astro project
#
# (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
#

ICEDEPENDENCIES = $(top_builddir)/lib/libastro.la \
	$(top_builddir)/ice/lib/libiceastro.la
ICELIBS = @ICELIBS@
ICELDADD = -L$(top_builddir)/lib -lastro \
	-L$(top_builddir)/ice/lib -liceastro $(ICELIBS)
ICECPPFLAGS = -I$(top_builddir)/ice/include -I$(top_builddir)/ice/slice \
	@ICEINCLUDE@

bin_PROGRAMS = snowguide 

noinst_HEADERS = display.h monitor.h guide.h

snowguide_SOURCES = display.cpp monitor.cpp guide.cpp			\
	guidecmd.cpp calcmd.cpp generalcmd.cpp monitorcmd.cpp 		\
	darkcmd.cpp imagecmd.cpp					\
	snowguide.cpp
snowguide_DEPENDENCIES = $(ICEDEPENDENCIES)
snowguide_LDADD = $(ICELDADD)
snowguide_CPPFLAGS = $(ICECPPFLAGS)

INTERVAL = 10

calibrate:	snowguide
	./snowguide --debug --star=349,252 --width=60 \
		--exposure=1 server GUIDESIM calibrate GP
aocalibrate:	snowguide
	./snowguide --debug --star=349,252 --width=60 \
		--exposure=1 server GUIDESIM calibrate AO
guide:	snowguide
	./snowguide --debug --star=349,252 --width=60 \
		--exposure=1 --interval=$(INTERVAL) server GUIDESIM guide

planetcalibrate:	snowguide
	./snowguide --debug --exposure=1 --star=320,240 --width=64 \
		--method=diff server GUIDESIM calibrate
planetguide:	snowguide
	./snowguide --debug --exposure=1 --star=320,240 --width=64 \
		--method=diff --interval=$(INTERVAL) server GUIDESIM guide

suncalibrate:	snowguide
	./snowguide --debug --exposure=1 \
		--method=diff server GUIDESIM calibrate
sunaocalibrate:	snowguide
	./snowguide --debug --exposure=1 \
		--method=diff server GUIDESIM calibrate AO
sunguide:	snowguide
	./snowguide --debug --exposure=1 \
		--method=diff --interval=$(INTERVAL) server GUIDESIM guide

