#!/bin/bash

#export HCLIB_FLAGS="--enable-production"
export LIBXML2_INCLUDE=/usr/include/libxml2
export LIBXML2_LIBS=/usr/lib/x86_64-linux-gnu
export BASE=/home/jmg3/degas/hcpp
# export HC_CUDA_FLAGS=--enable-cuda
export CC=gcc
export CXX=g++

################################################
#
# DO NOT MODIFY ANYTHING BELOW UNLESS YOU ARE
# CHANGING THE INSTALLATION PATH OF HCLIB
#
################################################

export hclib=${BASE}
export HCLIB_ROOT=${hclib}/hclib-install
export LD_LIBRARY_PATH=${HCLIB_ROOT}/lib:${LIBXML2_LIBS}:$LD_LIBRARY_PATH

if [ ! -z "${TBB_MALLOC}" ]; then
   export LD_LIBRARY_PATH=${TBB_MALLOC}:$LD_LIBRARY_PATH
fi
