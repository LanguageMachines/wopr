AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = wopr woprsm

wopr_SOURCES = \
	src/main.cc \
	src/util.h \
	src/util.cc \
	src/qlog.h \
	src/qlog.cc \
	src/Config.h \
	src/Config.cc \
	src/server.h \
	src/server.cc \
	src/runrunrun.h \
	src/runrunrun.cc \
	src/watcher.h \
	src/watcher.cc \
	src/tag.h \
	src/tag.cc \
	src/Classifier.h \
	src/Classifier.cc \
	src/Multi.h \
	src/elements.h \
	src/elements.cc \
	src/levenshtein.cc \
	src/levenshtein.h

woprsm_SOURCES = \
	src/woprsm.cc

man_MANS = man/wopr.1

EXTRA_DIST = \
	$(man_MANS) \
	etc/nscript \
	etc/nscript1 \
	bootstrap.sh
#	m4/check_symbol.m4 \
#	scripts/gen_otp.pl \
#	scripts/info.sh \
#	scripts/log.sh \
#	scripts/quit.sh \
#	scripts/remove.pl \
#	scripts/start.sh \
#	scripts/status.sh \
#	scripts/stop.sh

