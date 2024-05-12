//
// Created by root on 5/5/24.
//

#ifndef S7_COMMUNICATION_PLUGIN_H
#define S7_COMMUNICATION_PLUGIN_H
#include "neuron.h"
#include "siemens_plc_s7_net/typedef.h"
typedef struct tag_hash {
    char           name[50]; /* key (string is WITHIN the structure) */
    int32_t        value;
    UT_hash_handle hh; /* makes this structure hashable */
} tag_hash_t;
struct neu_plugin {
    neu_plugin_common_t common;

    int         fd;
    char        host[16];
    uint64_t    port;
    uint16_t    timeout;

    neu_event_timer_t *timer;
    neu_events_t      *events;

    siemens_plc_types_e plc_type;
    bool connected;
    uint8_t keep_alive_connection_count;
};

#endif // S7_COMMUNICATION_PLUGIN_H
