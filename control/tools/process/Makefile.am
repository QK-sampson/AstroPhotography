#
# Makefile.am -- tool to execute a process from an XML file
#
# (c) 2017 Prof Dr Andreas Müller, Hochschule Rapperswil
#
bin_PROGRAMS = astroprocess

astroprocess_SOURCES = astroprocess.cpp
astroprocess_DEPENDENCIES = $(top_builddir)/lib/libastro.la
astroprocess_LDADD = -L$(top_builddir)/lib -lastro

test:	astroprocess process.xml
	MallocGuardEdges=1 MallocScribble=1 \
	./astroprocess --debug process.xml 2>&1 | tee test.log

debug:	astroprocess process.xml
	libtool --mode=execute gdb ./astroprocess

macdebug:	astroprocess process.xml
	/usr/local/bin/libtool --mode=execute lldb ./astroprocess
