SOURCES = file.cpp main.cpp parser.cpp rule.cpp

lib_LTLIBRARIES = libnxlp.la

libnxlp_la_SOURCES = $(SOURCES)
libnxlp_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXLP_EXPORTS -I@top_srcdir@/build
libnxlp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxlp_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la

if USE_INTERNAL_EXPAT
libnxlp_la_CPPFLAGS+=-I@top_srcdir@/src/libexpat/libexpat
libnxlp_la_LIBADD += @top_srcdir@/src/libexpat/libexpat/libnxexpat.la
endif

EXTRA_DIST = \
	eventlog.cpp vss.cpp wevt.cpp \
	libnxlp.vcxproj libnxlp.vcxproj.filters \
	libnxlp.h
