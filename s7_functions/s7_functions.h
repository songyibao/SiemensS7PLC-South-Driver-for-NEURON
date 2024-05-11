//
// Created by root on 5/11/24.
//

#ifndef NEURON_S7_FUNCTIONS_H
#define NEURON_S7_FUNCTIONS_H
#include "../s7_communication_plugin.h"
int s7_plugin_connect(neu_plugin_t *plugin);
int s7_plugin_disconnect(neu_plugin_t *plugin);
void add_connection_status_checker(neu_plugin_t *plugin);
#endif // NEURON_S7_FUNCTIONS_H
