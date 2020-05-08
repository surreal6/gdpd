#!/bin/bash
git submodule update --init --recursive
cd src/godot-cpp
scons platform=$1 generate_bindings=yes
cd ../..
scons platform=$1
