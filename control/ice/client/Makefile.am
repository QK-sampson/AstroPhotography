#
# Makefile.am -- build ICE test client for the Astro project
#
# (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil
#
SUBDIRS = guide 

ICEDEPENDENCIES = $(top_builddir)/lib/libastro.la \
	$(top_builddir)/ice/lib/libiceastro.la
ICELIBS = @ICELIBS@
ICELDADD = -L$(top_builddir)/lib -lastro \
	-L$(top_builddir)/ice/lib -liceastro $(ICELIBS)
ICECPPFLAGS = -I$(top_builddir)/ice/include -I$(top_builddir)/ice/slice \
	@ICEINCLUDE@

bin_PROGRAMS = snowscan snowimages snowfocus snowmount snowrepo \
	snowtask snowproject snowinstrument snowao snowservers \
	snowevent snowconfig snowstream
noinst_PROGRAMS = snowflake 

snowconfig_SOURCES = snowconfig.cpp
snowconfig_DEPENDENCIES = $(ICEDEPENDENCIES)
snowconfig_LDADD = $(ICELDADD)
snowconfig_CPPFLAGS = $(ICECPPFLAGS)

snowflake_SOURCES = snowflake.cpp
snowflake_DEPENDENCIES = $(ICEDEPENDENCIES)
snowflake_LDADD = $(ICELDADD)
snowflake_CPPFLAGS = $(ICECPPFLAGS)

snowscan_SOURCES = snowscan.cpp
snowscan_DEPENDENCIES = $(ICEDEPENDENCIES)
snowscan_LDADD = $(ICELDADD)
snowscan_CPPFLAGS = $(ICECPPFLAGS)

snowimages_SOURCES = snowimages.cpp
snowimages_DEPENDENCIES = $(ICEDEPENDENCIES)
snowimages_LDADD = $(ICELDADD)
snowimages_CPPFLAGS = $(ICECPPFLAGS)

snowfocus_SOURCES = snowfocus.cpp
snowfocus_DEPENDENCIES = $(ICEDEPENDENCIES)
snowfocus_LDADD = $(ICELDADD)
snowfocus_CPPFLAGS = $(ICECPPFLAGS)

snowmount_SOURCES = snowmount.cpp
snowmount_DEPENDENCIES = $(ICEDEPENDENCIES)
snowmount_LDADD = $(ICELDADD)
snowmount_CPPFLAGS = $(ICECPPFLAGS)

snowrepo_SOURCES = snowrepo.cpp RepoReplicators.cpp
snowrepo_DEPENDENCIES = $(ICEDEPENDENCIES)
snowrepo_LDADD = $(ICELDADD)
snowrepo_CPPFLAGS = $(ICECPPFLAGS)

snowtask_SOURCES = snowtask.cpp
snowtask_DEPENDENCIES = $(ICEDEPENDENCIES)
snowtask_LDADD = $(ICELDADD)
snowtask_CPPFLAGS = $(ICECPPFLAGS)

snowproject_SOURCES = snowproject.cpp
snowproject_DEPENDENCIES = $(ICEDEPENDENCIES)
snowproject_LDADD = $(ICELDADD)
snowproject_CPPFLAGS = $(ICECPPFLAGS)

snowinstrument_SOURCES = snowinstrument.cpp
snowinstrument_DEPENDENCIES = $(ICEDEPENDENCIES)
snowinstrument_LDADD = $(ICELDADD)
snowinstrument_CPPFLAGS = $(ICECPPFLAGS)

snowao_SOURCES = snowao.cpp
snowao_DEPENDENCIES = $(ICEDEPENDENCIES)
snowao_LDADD = $(ICELDADD)
snowao_CPPFLAGS = $(ICECPPFLAGS)

snowservers_SOURCES = snowservers.cpp
snowservers_DEPENDENCIES = $(ICEDEPENDENCIES)
snowservers_LDADD = $(ICELDADD)
snowservers_CPPFLAGS = $(ICECPPFLAGS)

snowevent_SOURCES = snowevent.cpp
snowevent_DEPENDENCIES = $(ICEDEPENDENCIES)
snowevent_LDADD = $(ICELDADD)
snowevent_CPPFLAGS = $(ICECPPFLAGS)

snowstream_SOURCES = snowstream.cpp
snowstream_DEPENDENCIES = $(ICEDEPENDENCIES)
snowstream_LDADD = $(ICELDADD)
snowstream_CPPFLAGS = $(ICECPPFLAGS)

test:	snowflake
	./snowflake -d -m simulator

teststream:
	./snowstream --debug --exposure=1 server SIM 
