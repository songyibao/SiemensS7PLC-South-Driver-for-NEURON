//
// Created by SongYibao on 5/5/24.
//
/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/
#include <stdlib.h>
#include "neuron.h"
#include "tag_handle/tag_handle.h"
#include "s7_communication_plugin.h"
#include "s7_functions/s7_functions.h"

static neu_plugin_t *driver_open(void);

static int driver_close(neu_plugin_t *plugin);
static int driver_init(neu_plugin_t *plugin, bool load);
static int driver_uninit(neu_plugin_t *plugin);
static int driver_start(neu_plugin_t *plugin);
static int driver_stop(neu_plugin_t *plugin);
static int driver_config(neu_plugin_t *plugin, const char *config);
static int driver_request(neu_plugin_t *plugin, neu_reqresp_head_t *head, void *data);

static int driver_tag_validator(const neu_datatag_t *tag);
static int driver_validate_tag(neu_plugin_t *plugin, neu_datatag_t *tag);
static int driver_group_timer(neu_plugin_t *plugin, neu_plugin_group_t *group);
static int driver_write(neu_plugin_t *plugin, void *req, neu_datatag_t *tag, neu_value_u value);
static int driver_write_tags(neu_plugin_t *plugin, void *req, UT_array *tags);
static int driver_del_tags(neu_plugin_t *plugin, int n_tag);
static const neu_plugin_intf_funs_t plugin_intf_funs = {
    .open    = driver_open,
    .close   = driver_close,
    .init    = driver_init,
    .uninit  = driver_uninit,
    .start   = driver_start,
    .stop    = driver_stop,
    .setting = driver_config,
    .request = driver_request,

    .driver.validate_tag  = driver_validate_tag,
    .driver.group_timer   = driver_group_timer,
    .driver.write_tag     = driver_write,
    .driver.tag_validator = driver_tag_validator,
    .driver.write_tags    = driver_write_tags,
    .driver.add_tags      = NULL,
    .driver.load_tags     = NULL,
    .driver.del_tags      = driver_del_tags,
};

const neu_plugin_module_t neu_plugin_module = {
    .version         = NEURON_PLUGIN_VER_1_0,
    .schema          = "S7-Communication",
    .module_name     = "S7-Communication",
    .module_descr    = "This plugin is used to connect Siemens S7 series PLC equipment",
    .module_descr_zh = "该插件用于连接西门子 S7 系列 PLC 设备",
    .intf_funs       = &plugin_intf_funs,
    .kind            = NEU_PLUGIN_KIND_SYSTEM,
    .type            = NEU_NA_TYPE_DRIVER,
    .display         = true,
    .single          = false,
};

static neu_plugin_t *driver_open(void)
{
    nlog_debug("\n==============================driver_open"
               "===========================\n");
    neu_plugin_t *plugin = calloc(1, sizeof(neu_plugin_t));

    neu_plugin_common_init(&plugin->common);

    return plugin;
}

static int driver_close(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_close"
               "===========================\n");
    free(plugin);

    return 0;
}

static int driver_init(neu_plugin_t *plugin, bool load)
{
    plog_debug(plugin,
               "\n==============================driver_init"
               "===========================\n");
    (void) load;
    plugin->events = neu_event_new();

    add_connection_status_checker(plugin);

    plugin->fd     = -1;
    plugin->connected = false;
    plugin->plc_type = S1500;
    plog_notice(plugin, "%s init success", plugin->common.name);
    return 0;
}

static int driver_uninit(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_uninit"
               "===========================\n");
    plog_notice(plugin, "%s uninit start", plugin->common.name);


    neu_event_close(plugin->events);
    s7_plugin_disconnect(plugin);
    plugin->fd = -1;
    plog_notice(plugin, "%s uninit success", plugin->common.name);
    return 0;
}

static int driver_start(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_start"
               "===========================\n");
    return 0;
}

static int driver_stop(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_stop"
               "===========================\n");
    s7_plugin_disconnect(plugin);
    plog_notice(plugin, "%s stop success", plugin->common.name);
    return 0;
}

static int driver_config(neu_plugin_t *plugin, const char *config)
{
    plog_debug(plugin,
               "\n==============================driver_config"
               "===========================\n");
    s7_plugin_disconnect(plugin);
    int   ret       = 0;
    char *err_param = NULL;

    //    neu_json_elem_t slot = { .name = "slot", .t = NEU_JSON_INT };

    neu_json_elem_t port           = { .name = "port", .t = NEU_JSON_INT };
    neu_json_elem_t timeout        = { .name = "timeout", .t = NEU_JSON_INT };
    neu_json_elem_t host           = { .name = "host", .t = NEU_JSON_STR, .v.val_str = NULL };
    neu_json_elem_t plc_type             = { .name = "plc_type", .t = NEU_JSON_INT };
    ret = neu_parse_param((char *) config, &err_param, 4, &port, &host, &timeout,&plc_type);

    if (ret != 0) {
        plog_error(plugin, "config: %s, decode error: %s", config, err_param);
        free(err_param);
        if (host.v.val_str != NULL) {
            free(host.v.val_str);
        }
        return -1;
    }

    if (timeout.v.val_int <= 0) {
        plog_error(plugin, "config: %s, set timeout error: %s", config, err_param);
        free(err_param);
        return -1;
    }
    if (plc_type.v.val_int <1 || plc_type.v.val_int > 6) {
        plog_error(plugin, "config: %s, set plc_type error: %s, plc_type should be in 1-6", config, err_param);
        free(err_param);
        return -1;
    }

    plog_notice(plugin, "config: host: %s, port: %" PRId64, host.v.val_str, port.v.val_int);

    plugin->plc_type = plc_type.v.val_int;
    plugin->port = port.v.val_int;
    strcpy(plugin->host, host.v.val_str);
    free(host.v.val_str);
    // timeout 单位是毫秒
    plugin->timeout = timeout.v.val_int;
    return 0;
}

static int driver_request(neu_plugin_t *plugin, neu_reqresp_head_t *head, void *data)
{
    plog_debug(plugin,
               "\n==============================driver_request"
               "===========================\n");
    (void) plugin;
    (void) head;
    (void) data;
    return 0;
}
static int driver_tag_validator(const neu_datatag_t *tag)
{
    return 0;
}
static int driver_validate_tag(neu_plugin_t *plugin, neu_datatag_t *tag)
{
    plog_debug(plugin,
               "\n==============================driver_validate_tag"
               "===========================\n");
    // 打印tag的信息
    plog_debug(plugin,
               "tag: %s, address: %s, attribute: %d, type: %d, "
               "precision: %d, decimal: %f, description: %s",
               tag->name, tag->address, tag->attribute, tag->type, tag->precision, tag->decimal, tag->description);
//    // 创建 plctag 库的 plc_tag 句柄
//    int32_t plc_tag = create_and_add_plc_tag(plugin, tag);
//    if (plc_tag != PLCTAG_STATUS_OK) {
//        plog_debug(plugin, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(plc_tag));
//    }

    return 0;
}

static int driver_group_timer(neu_plugin_t *plugin, neu_plugin_group_t *group)
{
    plog_debug(plugin,
               "\n\n==============================driver_group_timer"
               "===========================\n");
    if(plugin->connected == false){
        if(s7_plugin_connect(plugin)!=0){
            plog_error(plugin, "driver_group_timer:Connect failed");
            return 0;
        }
    }
    utarray_foreach(group->tags, neu_datatag_t *, tag)
    {
        handle_tag(plugin, tag, group);
    }
    return 0;
}
static int driver_write(neu_plugin_t *plugin, void *req, neu_datatag_t *tag, neu_value_u value)
{
    plog_debug(plugin,
               "\n==============================driver_write"
               "===========================\n");
    plog_debug(plugin, "write tag: %s, address: %s", tag->name, tag->address);
    return 0;
}

static int driver_write_tags(neu_plugin_t *plugin, void *req, UT_array *tags)
{
    plog_debug(plugin,
               "\n==============================driver_write_tags"
               "===========================\n");
    return 0;
}
static int driver_del_tags(neu_plugin_t *plugin, int n_tag)
{
    plog_debug(plugin,
               "\n==============================driver_del_tags"
               "===========================\n");
    plog_debug(plugin, "deleting %d tags", n_tag);
    return 0;
}