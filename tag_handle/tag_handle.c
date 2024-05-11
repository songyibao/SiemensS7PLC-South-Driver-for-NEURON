//
// Created by root on 5/6/24.
//
#include "tag_handle.h"
#include "../siemens_plc_s7_net/siemens_s7.h"
#include "../s7_functions/s7_functions.h"
int read_tag(neu_plugin_t *plugin, neu_plugin_group_t *group,
             neu_datatag_t *tag, neu_dvalue_t *dvalue)
{
    dvalue->type               = tag->type;
    s7_error_code_e error_code = 0;
    int             res        = 0;
    if (plugin->connected == true) {
        // 开始连接plc读取数据的时刻
        uint64_t read_tms = neu_time_ms();

        switch (tag->type) {
        case NEU_TYPE_INT8:
            byte int8;
            error_code = s7_read_byte(plugin->fd, tag->address, &int8);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.i8 = int8;
            }
            break;
        case NEU_TYPE_WORD:
        case NEU_TYPE_UINT8:
            byte uint8;
            error_code = s7_read_byte(plugin->fd, tag->address, &uint8);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.u8 = uint8;
            }
            break;
        case NEU_TYPE_INT16:
            short int16;
            error_code = s7_read_short(plugin->fd, tag->address, &int16);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.i16 = int16;
            }
            break;
        case NEU_TYPE_UINT16:
            ushort uint16;
            error_code = s7_read_ushort(plugin->fd, tag->address, &uint16);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.u16 = uint16;
            }
            break;
        case NEU_TYPE_INT32:
            int32 res_int32;
            error_code = s7_read_int32(plugin->fd, tag->address, &res_int32);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.i32 = res_int32;
            }
            break;
        case NEU_TYPE_DWORD:
        case NEU_TYPE_UINT32:
            uint32 res_uint32;
            error_code = s7_read_uint32(plugin->fd, tag->address, &res_uint32);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.u32 = res_uint32;
            }
            break;
        case NEU_TYPE_INT64:
            int64 res_int64;
            error_code = s7_read_int64(plugin->fd, tag->address, &res_int64);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.i64 = res_int64;
            }
            break;
        case NEU_TYPE_LWORD:
        case NEU_TYPE_UINT64:
            uint64 res_uint64;
            error_code = s7_read_uint64(plugin->fd, tag->address, &res_uint64);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.u64 = res_uint64;
            }
            break;
        case NEU_TYPE_FLOAT:
            float res_float;
            error_code = s7_read_float(plugin->fd, tag->address, &res_float);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.f32 = res_float;
            }
            break;
        case NEU_TYPE_DOUBLE:
            double res_double;
            error_code = s7_read_double(plugin->fd, tag->address, &res_double);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.d64 = res_double;
            }
            break;
        case NEU_TYPE_BOOL:
            bool res_bool;
            error_code = s7_read_bool(plugin->fd, tag->address, &res_bool);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                dvalue->value.boolean = res_bool;
            }
            break;
        case NEU_TYPE_STRING:
            char *res_str=NULL;
            error_code = s7_read_string(plugin->fd, tag->address, 128,&res_str);
            if (error_code == S7_ERROR_CODE_SUCCESS) {
                strcpy(dvalue->value.str, res_str);
            }
            if(res_str!=NULL){
                free(res_str);
            }
            break;
        case NEU_TYPE_BIT:
        default:
            dvalue->type = NEU_TYPE_ERROR;
            dvalue->value.i32 = NEU_ERR_PLUGIN_TAG_NOT_ALLOW_READ;
            break;
        }
        if(error_code!=S7_ERROR_CODE_SUCCESS){
            dvalue->type      = NEU_TYPE_ERROR;
            dvalue->value.i32 = NEU_ERR_PLUGIN_TAG_NOT_READY;
        }
        uint64_t read_delay = neu_time_ms() - read_tms;
        NEU_PLUGIN_UPDATE_METRIC(plugin, NEU_METRIC_LAST_RTT_MS, read_delay,
                                 NULL);
    } else {
        dvalue->type      = NEU_TYPE_ERROR;
        dvalue->value.i32 = NEU_ERR_PLUGIN_DISCONNECTED;
        // 尝试重新连接
        plog_debug(plugin, "Trying to reconnect %s\n", tag->address);
        if(s7_plugin_connect(plugin)==S7_ERROR_CODE_SUCCESS){
            plog_debug(plugin, "Reconnect %s success\n", tag->address);
        }else{
        }
        res = -1;
        goto exit;
    }
    return res;
exit:
    return res;
}
void handle_tag(neu_plugin_t *plugin, neu_datatag_t *tag,
                neu_plugin_group_t *group)
{
    //    plog_debug(plugin,
    //               "tag: %s, address: %s, attribute: %d, type: %d, "
    //               "precision: %d, decimal: %f, description: %s",
    //               tag->name, tag->address, tag->attribute, tag->type,
    //               tag->precision, tag->decimal, tag->description);
    neu_dvalue_t dvalue = { 0 };
    read_tag(plugin, group, tag,&dvalue);
    plugin->common.adapter_callbacks->driver.update(
        plugin->common.adapter, group->group_name, tag->name, dvalue);
//    switch (tag->type) {
//    case NEU_TYPE_INT8:
//    case NEU_TYPE_UINT8:
//    case NEU_TYPE_BIT:
//    case NEU_TYPE_BOOL:
//        read_tag(plugin, group, tag, 1, &dvalue);
//        break;
//    case NEU_TYPE_INT16:
//    case NEU_TYPE_UINT16:
//    case NEU_TYPE_WORD:
//        read_tag(plugin, group, tag, 2, &dvalue);
//        break;
//    case NEU_TYPE_INT32:
//    case NEU_TYPE_UINT32:
//    case NEU_TYPE_FLOAT:
//    case NEU_TYPE_DWORD:
//        read_tag(plugin, group, tag, 4, &dvalue);
//        break;
//    case NEU_TYPE_INT64:
//    case NEU_TYPE_UINT64:
//    case NEU_TYPE_DOUBLE:
//    case NEU_TYPE_LWORD:
//        read_tag(plugin, group, tag, 8, &dvalue);
//        break;
//    case NEU_TYPE_STRING:
//        read_tag(plugin, group, tag, 0, &dvalue);
//        break;
//    default:
//        break;
//    }
}