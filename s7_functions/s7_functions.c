//
// Created by root on 5/11/24.
//
#include "s7_functions.h"
#include <stdbool.h>
#include "../siemens_plc_s7_net/siemens_s7.h"

void print_error_message(s7_error_code_e error_code) {
    switch(error_code) {
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
int s7_plugin_connect(neu_plugin_t *plugin){
    bool ret = s7_connect(plugin->host, plugin->port, plugin->plc_type, &plugin->fd);
    if (ret && plugin->fd > 0)
    {
        plugin->connected = true;
        plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
        plog_debug(plugin, "Connected to PLC");
        return 0;
    }else{
        plugin->connected = false;
        plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
        plog_error(plugin, "Failed to connect to PLC,errcode:%d",ret);
        print_error_message(ret);
        return -1;
    }
}
int s7_plugin_disconnect(neu_plugin_t *plugin){
    bool ret = s7_disconnect(plugin->fd);
    if (ret)
    {
        plugin->connected = false;
        plog_debug(plugin, "Disconnected from PLC");
        return 0;
    }
}
int check_connection_status_callback(void *data)
{
    neu_plugin_t *plugin = (neu_plugin_t *) data;
    int fd = -1;

    bool ret = s7_connect(plugin->host, plugin->port, plugin->plc_type, &fd);
    if (ret==true)
    {

        plugin->connected = true;
        plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
        plog_debug(plugin, "Checker: PLC is connected");
//        int i_val = 0;
//        s7_read_int32(fd, "MD0", &i_val);
//        plog_debug(plugin,"Read\t %s \tint32:\t %d\n", "MD0",i_val);

//        s7_disconnect(fd);
        return 0;
    }else{
        plugin->connected = false;
        plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
        plog_error(plugin,"host:%s, port:%ld, plc_type:%d", plugin->host, plugin->port, plugin->plc_type);
        plog_error(plugin, "Checker: Failed to connect to PLC,errcode:%d,fd:%d",ret,fd);
        print_error_message(ret);
        return -1;
    }
}
void add_connection_status_checker(neu_plugin_t *plugin)
{
    neu_event_timer_param_t param = { .second      = 3,
                                      .millisecond = 0,
                                      .cb          = check_connection_status_callback,
                                      .usr_data    = (void *) plugin,
                                      .type        = NEU_EVENT_TIMER_NOBLOCK };

    plugin->timer = neu_event_add_timer(plugin->events, param);
}