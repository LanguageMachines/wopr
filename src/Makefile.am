# $Id: Makefile.am 16386 2013-07-23 14:17:01Z sloot $
# $URL: https://ilk.uvt.nl/svn/sources/tscan/trunk/src/Makefile.am $

AM_CPPFLAGS = -I@top_srcdir@/include
AM_CXXFLAGS = -std=c++11

bin_PROGRAMS = wopr woprsm woprst

wopr_SOURCES = Classifier.cc \
	generator.cc \
	prededit.cc \
	tinyxml.cpp \
	wex.cc \
	Config.cc \
	lcontext.cc \
        qlog.cc \
        tinyxmlerror.cpp \
	Context.cc \
	levenshtein.cc \
	runrunrun.cc \
	tinyxmlparser.cpp \
	elements.cc \
	main.cc \
	server.cc \
	tr.cc \
	focus.cc \
	ngrams.cc \
	SocketBasics.cc  \
	util.cc \
	general_test.cc \
	ngram_server.cc \
	tag.cc \
        watcher.cc

woprsm_SOURCES = \
	woprsm.cc

woprst_SOURCES = \
	qlog.cc \
	util.cc \
	SocketBasics.cc \
	woprst.cc
