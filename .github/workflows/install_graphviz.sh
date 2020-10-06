#!/bin/bash

set -e
pushd /tmp

curl -L "https://www2.graphviz.org/Packages/stable/portable_source/graphviz-2.44.1.tar.gz" -o graphviz.tar.gz
tar xzf graphviz.tar.gz
rm graphviz.tar.gz

pushd graphviz-2.44.1

export CXXFLAGS+=' -fPIC -fpermissive'
export CFLAGS+=' -fPIC'
export PS2PDF=true 
/usr/bin/scl enable devtoolset-8 -- ./configure \
  --enable-static --disable-shared \
  --enable-ltdl=no \
  --enable-swig=no \
  --enable-sharp=no \
  --enable-d=no \
  --enable-go=no \
  --enable-guile=no \
  --enable-io=no \
  --enable-java=no \
  --enable-javascript=no \
  --enable-lua=no \
  --enable-ocaml=no \
  --enable-perl=no \
  --enable-php=no \
  --enable-python=no \
  --enable-python2=no \
  --enable-python3=no \
  --enable-r=no \
  --enable-ruby=no \
  --enable-tcl=no \
  --with-efence=no \
  --with-x=no \
  --with-expat=no \
  --with-devil=no \
  --with-webp=no \
  --with-poppler=no \
  --with-rsvg=no \
  --with-ghostscript=no \
  --with-visio=no \
  --with-pangocairo=no \
  --with-lasi=no \
  --with-glitz=no \
  --with-freetype2=no \
  --with-fontconfig=no \
  --with-gdk=no \
  --with-gdk-pixbuf=no \
  --with-gtk=no \
  --with-gtkgl=no \
  --with-gtkglext=no \
  --with-gts=no \
  --with-ann=no \
  --with-glade=no \
  --with-ming=no \
  --with-qt=no \
  --with-quartz=no \
  --with-gdiplus=no \
  --with-libgd=no \
  --with-gdincludedir=no \
  --with-gdlibdir=no \
  --with-glut=no \
  --with-sfdp=yes \
  --with-smyrna=no \
  --with-ortho=no \
  --with-digcola=no \
  --with-ipsepcola=no \
  --prefix=/home/paraview/buildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuild/install \
  --exec-prefix=/home/paraview/buildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuildbuild/install
/usr/bin/scl enable devtoolset-8 -- make -j4
/usr/bin/scl enable devtoolset-8 -- make install
popd
rm -r graphviz-2.44.1
popd
