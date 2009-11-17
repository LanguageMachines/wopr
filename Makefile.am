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
	src/levenshtein.h \
	src/generator.h \
	src/generator.cc \
	src/lcontext.cc \
	src/lcontext.h \
	src/focus.h \
	src/focus.cc \
	src/tr.h \
	src/tr.cc \
	src/general_test.h \
	src/general_test.cc \
	src/MersenneTwister.h

woprsm_SOURCES = \
	src/woprsm.cc

man_MANS = man/wopr.1

EXTRA_DIST = \
	$(man_MANS) \
	etc/nscript \
	etc/nscript1 \
	etc/generator.script \
	etc/graph_wplex.pl \
	etc/examine_px.pl \
	bootstrap.sh \
	web/generator/generator.css \
	web/generator/generator.php \
	web/generator/generator.script \
	web/generator/index.html \
	web/generator/js/generator.js \
	web/generator/js/mootools-1.2.1-core.js

