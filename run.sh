#!/bin/sh
rm -rf ./build
# shellcheck disable=SC2164
mkdir build && cd build && cmake .. && make
echo "build done,coping plugin ..."
cp -r ./libplugin-S7-Communication.so /software/neuron/build/plugins/
cp -r ./libplugin-S7-Communication.so /opt/neuron/plugins/

cp -r ../S7-Communication.json /opt/neuron/plugins/schema/
cp -r ../S7-Communication.json /software/neuron/build/plugins/schema/
echo "copy done"