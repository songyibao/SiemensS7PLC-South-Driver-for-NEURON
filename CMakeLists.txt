#
# Copyright 2018 Capitar IT Group BV <info@capitar.com>
# Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.13)
set(LIBRARY_OUTPUT_PATH "./")
file(COPY /software/neuron/plugins/siemens/S7-Communication.json DESTINATION
        /software/neuron/build/plugins/schema)
project(S7-Communication)


add_library(plugin-S7-Communication SHARED s7_communication_plugin.c
        siemens_plc_s7_net/siemens_s7.c
        siemens_plc_s7_net/siemens_s7_comm.c
        siemens_plc_s7_net/siemens_helper.c
        siemens_plc_s7_net/socket.c
        siemens_plc_s7_net/utill.c
        siemens_plc_s7_net/dynstr.c
        s7_functions/s7_functions.c
        tag_handle/tag_handle.c
)
target_include_directories(plugin-S7-Communication PRIVATE
        /software/neuron/include/neuron
)
target_link_libraries(plugin-S7-Communication PRIVATE /usr/local/lib/neuron/libneuron-base.so)

