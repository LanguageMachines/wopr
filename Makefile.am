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
	src/tinyxmlerror.cpp \
	src/PDT.h \
	src/Expert.h

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
	etc/acc_gt.pl \
	etc/check_spelerr.pl \
	etc/combine2.pl \
	etc/compare_mg_px.pl \
	etc/compare_ngp_srilm.pl \
	etc/compare_ngt_srilm.pl \
	etc/compare_ngt_srilm_m.pl \
	etc/compare_px_srilm.pl \
	etc/compare_px_srilm_m.pl \
	etc/compare_pxs_srilm.pl \
	etc/confusion.pl \
	etc/create_cg_f_scatterplot.pl \
	etc/create_px_scatterplot.pl \
	etc/create_pxs_scatterplot.pl \
	etc/cstats.pl \
	etc/data2latex.pl \
	etc/data_pdt2_2latex.pl \
	etc/data_pdt2_extract.pl \
	etc/data_pdt_2latex.pl \
	etc/dti_spelerr.pl \
	etc/examine_mc.pl \
	etc/examine_md.pl \
	etc/examine_mg.pl \
	etc/examine_px.pl \
	etc/examine_px_cg.pl \
	etc/examine_pxs.pl \
	etc/examine_srilm.pl \
	etc/extract_mg_px.pl \
	etc/extract_px_px.pl \
	etc/filter_kvs.pl \
	etc/fix_pdt2.pl \
	etc/graph_wplex.pl \
	etc/make_spelerr.pl \
	etc/pplx_ngt.pl \
	etc/pplx_px.pl \
	etc/pxs2svg.pl \
	etc/create_algo_plots.sh \
	etc/create_plots.sh \
	etc/create_wex_plots.sh \
	etc/do_algos.sh \
	etc/do_all_mg.sh \
	etc/do_pplxs.sh \
	etc/do_weps.sh \
	etc/watch_all.sh \
	etc/watch_ceto.sh \
	etc/watch_wopr.sh \
	etc/ld.py \
	etc/post_correct.py \
	etc/ps_wopr.py \
	etc/tok.py \
	etc/tree2graph.py \
	etc/do_algos.wopr_script \
	etc/do_compare_mg_px.script \
	etc/do_lcontext.wopr_script \
	etc/do_pdt2.wopr_script \
	etc/do_pplxs.wopr_script \
	etc/generator.script \
	etc/make_data.script \
	etc/make_ibase.script \
	etc/nscript \
	etc/script \
	etc/wep.wopr_script \
	bootstrap \
	web/generator/generator.css \
	web/generator/generator.php \
	web/generator/generator.script \
	web/generator/index.html \
	web/generator/js/generator.js \
	web/generator/js/mootools-1.2.1-core.js \
	web/pdt/index.html \
	web/pdt/jquery-1.7.1.min.js \
	web/pdt/wopr.php \
	web/compoli/README.txt \
	web/compoli/add_txt.php \
	web/compoli/db.php \
	web/compoli/index.html \
	web/compoli/initdb.php \
	web/compoli/jquery-1.7.1.min.js \
	web/compoli/submitted.sqll \
	web/compoli/wopr.php \
	doc/woprdoc.tex \
	doc/acronyms.tex \
	doc/defs.tex \
	doc/tikzdefs.tex \
	doc/32376_mem.pdf \
	doc/26251_mem.pdf \
	doc/NY_pplx_-a1+D.pdf \
	doc/NY_pplx_slirm.pdf \
	stimpy/confusedata.py \
	stimpy/dir_to_tspy.bash \
	stimpy/multi.py \
	stimpy/run_exps.bash \
	stimpy/stimpy.py