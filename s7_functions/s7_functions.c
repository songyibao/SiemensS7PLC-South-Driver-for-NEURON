//
// Created by root on 5/11/24.
//
#include "s7_functions.h"
#include <stdbool.h>
#include "../siemens_plc_s7_net/siemens_s7.h"
#include "../siemens_plc_s7_net/socket.h"
#include "../siemens_plc_s7_net/siemens_s7_comm.h"

void print_error_message(s7_error_code_e error_code) {
    switch (error_code) {
        case S7_ERROR_CODE_SUCCESS:
            printf("成功\n");
            break;
        case S7_ERROR_CODE_FAILED:
            printf("错误\n");
            break;
        case S7_ERROR_CODE_FW_ERROR:
            printf("发生了异常，具体信息查找Fetch/Write协议文档\n");
            break;
        case S7_ERROR_CODE_ERROR_0006:
            printf("当前操作的数据类型不支持\n");
            break;
        case S7_ERROR_CODE_ERROR_000A:
            printf("尝试读取不存在的DB块数据\n");
            break;
        case S7_ERROR_CODE_WRITE_ERROR:
            printf("写入数据异常\n");
            break;
        case S7_ERROR_CODE_DB_SIZE_TOO_LARGE:
            printf("DB块数据无法大于255\n");
            break;
        case S7_ERROR_CODE_READ_LENGTH_MAST_BE_EVEN:
            printf("读取的数据长度必须为偶数\n");
            break;
        case S7_ERROR_CODE_DATA_LENGTH_CHECK_FAILED:
            printf("数据块长度校验失败，请检查是否开启put/get以及关闭db块优化\n");
            break;
        case S7_ERROR_CODE_READ_LENGTH_OVER_PLC_ASSIGN:
            printf("读取的数据范围超出了PLC的设定\n");
            break;
        case S7_ERROR_CODE_READ_LENGTH_CANNT_LARAGE_THAN_19:
            printf("读取的数组数量不允许大于19\n");
            break;
        case S7_ERROR_CODE_MALLOC_FAILED:
            printf("内存分配错误\n");
            break;
        case S7_ERROR_CODE_INVALID_PARAMETER:
            printf("参数错误\n");
            break;
        case S7_ERROR_CODE_PARSE_ADDRESS_FAILED:
            printf("地址解析错误\n");
            break;
        case S7_ERROR_CODE_BUILD_CORE_CMD_FAILED:
            printf("构建核心命令错误\n");
            break;
        case S7_ERROR_CODE_SOCKET_SEND_FAILED:
            printf("发送命令错误\n");
            break;
        case S7_ERROR_CODE_RESPONSE_HEADER_FAILED:
            printf("响应包头不完整错误\n");
            break;
        default:
            printf("未知错误\n");
            break;
    }
}

int s7_plugin_connect(neu_plugin_t *plugin) {
    if (plugin->connected == true) {
        return -1;
    }
    bool ret = false;
    plugin->fd = socket_open_tcp_client_socket(plugin->host, plugin->port);
    if (plugin->fd < 0) {
        plog_debug(plugin, "Failed to open socket");
        return -1;
    }
    s7_initialization(plugin->plc_type, plugin->host);
    // index 0 表示秒数, index 1 表示微妙数
    struct timeval timeout = {0, plugin->timeout};
    ret = setsockopt(plugin->fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &timeout, sizeof(timeout));
    if (ret != 0) {
        plog_error(plugin, "Setsockopt false");
        return -1;
    }
    ret = setsockopt(plugin->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    if (ret != 0) {
        plog_error(plugin, "Setsockopt false");
        return -1;
    }
    ret = initialization_on_connect(plugin->fd);

    if (ret == true) {
        plugin->connected = true;
        plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
//        plog_debug(plugin, "Connected to PLC");
        return 0;
    } else {
        plog_error(plugin,"host:%s, port:%ld, plc_type:%d", plugin->host, plugin->port, plugin->plc_type);
        plog_error(plugin, "Failed to connect to PLC,errcode:%d", ret);
        print_error_message(ret);
        s7_plugin_disconnect(plugin);
        return -1;
    }
}

int s7_plugin_disconnect(neu_plugin_t *plugin) {
    int res = close(plugin->fd);
    if (res == 0) {
        plugin->connected = false;
        plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
        plugin->fd = -1;
    } else {
        plog_error(plugin, "Failed to close plugin->fd:%d", plugin->fd);
    }
    return res;
}

int check_connection_status_callback(void *data) {
    neu_plugin_t *plugin = (neu_plugin_t *) data;
    if(plugin->keep_alive_connection_count==1){
        return 0;
    }
    plugin->keep_alive_connection_count=1;
    int res = S7_ERROR_CODE_FAILED;
    if (plugin->connected == true) {
        char *type = NULL;
        res = s7_read_plc_type(plugin->fd, &type);
        plugin->keep_alive_connection_count=0;
        if (res == S7_ERROR_CODE_FAILED) {
            s7_plugin_disconnect(plugin);

            plog_debug(plugin,"Checker: Connection interrupted");
        }
    } else {
        res = s7_plugin_connect(plugin);
        plugin->keep_alive_connection_count=0;
        if(res == S7_ERROR_CODE_SUCCESS){
            plugin->connected = true;
            plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
            plog_debug(plugin, "Checker: PLC is connected");
        }else{
            s7_plugin_disconnect(plugin);
        }
    }
}

void add_connection_status_checker(neu_plugin_t *plugin) {
    neu_event_timer_param_t param = {.second      = 2,
            .millisecond = 0,
            .cb          = check_connection_status_callback,
            .usr_data    = (void *) plugin,
            .type        = NEU_EVENT_TIMER_NOBLOCK};

    plugin->timer = neu_event_add_timer(plugin->events, param);
}