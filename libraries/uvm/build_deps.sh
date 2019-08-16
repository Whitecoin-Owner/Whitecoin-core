#!/bin/bash
git submodule update --init --recursive
cd deps/jsondiff-cpp && cmake -DCMAKE_BUILD_TYPE=Release . && make && cd ../..

