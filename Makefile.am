ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include doc etc web m4 man stimpy

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS README

ChangeLog: $(top_srcdir)/NEWS
	git pull; git2cl > ChangeLog
