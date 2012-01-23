ACLOCAL_AMFLAGS = -I m4 --install

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = wopr woprsm woprst

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
	src/Context.h \
	src/Context.cc \
	src/prededit.h \
	src/prededit.cc \
	src/lcontext.cc \
	src/lcontext.h \
	src/focus.h \
	src/focus.cc \
	src/tr.h \
	src/tr.cc \
	src/general_test.h \
	src/general_test.cc \
	src/ngram_elements.h \
	src/ngrams.h \
	src/ngrams.cc \
	src/ngram_server.h \
	src/ngram_server.cc \
	src/wex.h \
	src/wex.cc \
	src/SocketBasics.h \
	src/SocketBasics.cc \
	src/MersenneTwister.h \
	src/cache.h \
	src/tinyxml.h \
	src/tinyxml.cpp \
	src/tinyxmlparser.cpp \
	src/tinyxmlerror.cpp

woprsm_SOURCES = \
	src/woprsm.cc

woprst_SOURCES = \
	src/qlog.h \
	src/qlog.cc \
	src/util.h \
	src/util.cc \
	src/SocketBasics.h \
	src/SocketBasics.cc \
	src/cache.h \
	src/woprst.cc

man_MANS = man/wopr.1

EXTRA_DIST = \
	$(man_MANS) \
	etc/nscript \
	etc/nscript1 \
	etc/generator.script \
	etc/graph_wplex.pl \
	etc/examine_px.pl \
	etc/examine_pxs.pl \
	etc/compare_px_srilm.pl \
	etc/compare_ngt_srilm.pl \
	etc/compare_ngp_srilm.pl \
	etc/examine_md.pl \
	etc/examine_mc.pl \
	etc/pplx_px.pl \
	bootstrap \
	web/generator/generator.css \
	web/generator/generator.php \
	web/generator/generator.script \
	web/generator/index.html \
	web/generator/js/generator.js \
	web/generator/js/mootools-1.2.1-core.js \
	doc/woprdoc.tex \
	doc/acronyms.tex \
	doc/defs.tex \
	doc/tikzdefs.tex \
	doc/32376_mem.pdf \
	doc/26251_mem.pdf \
	doc/NY_pplx_-a1+D.pdf \
	doc/NY_pplx_slirm.pdf

