# bootstrap - script to bootstrap the distribution rolling engine

# usage:
#  $ sh ./bootstrap && ./configure && make dist[check]
#
# this yields a tarball which one can install doing
#
#  $ tar zxf PACKAGENAME-*.tar.gz
#  $ cd PACKAGENAME-*
#  $ ./configure
#  $ make
#  # make install

# requirements:
#  GNU autoconf, from e.g. ftp.gnu.org:/pub/gnu/autoconf/
#  GNU automake, from e.g. http://ftp.gnu.org/gnu/automake/

automake=automake
aclocal=aclocal

# if you want to autogenerate a ChangeLog form svn:
#
#  git2cl

$aclocal -I m4


if $automake --version|head -1 |grep ' 1\.[4-8]'; then
    echo "automake 1.4-1.8 is active. You should use automake 1.9 or later"
    if test -f /etc/debian_version; then
        echo " sudo apt-get install automake1.9"
        echo " sudo update-alternatives --config automake"
    fi
    exit 1
fi

if $aclocal --version|head -1 |grep ' 1\.[4-8]'; then
    echo "aclocal 1.4-1.8 is active. You should use aclocal 1.9 or later"
    if test -f /etc/debian_version; then
        echo " sudo apt-get install aclocal1.9"
        echo " sudo update-alternatives --config aclocal"
    fi
    exit 1
fi

# Debian automake package installs as automake-version.  Use this
# to make sure the right automake is being used.
# if not installed, use: apt-get install automake1.9

AUTOMAKE=automake ACLOCAL=aclocal autoreconf --install \
      --symlink

# add --make if you want to run "make" too.

# autoreconf should run something like:
#
# aclocal-1.9 \
#     && automake-1.9 --add-missing --verbose --gnu \
#     && autoconf
