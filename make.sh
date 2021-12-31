#!/bin/bash

git submodule init
git submodule update
mkdir -p plot
cp fcpp/src/extras/plotter/plot.asy plot/

if [ "$1" == "sim" ]; then
    fcpp/src/make.sh gui run -O -DFCPP_SYSTEM=FCPP_SYSTEM_EMBEDDED graphic
else
    fcpp/src/make.sh "$@"
fi
