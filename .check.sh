#!/bin/sh
# test for Linux usage, if so, require -lbsd because gnulibc is crap
if [ `uname -s` == Linux ]; then
	echo -n "-lbsd"
fi
