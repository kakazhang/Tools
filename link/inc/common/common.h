#ifndef COMMON_H
#define COMMON_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#pragma pack(1)
typedef union {
    struct {
        uint16_t usLength:10;
        uint16_t usVersion:6;
    } u;

    uint16_t  usData;
} U_LV;

typedef union {
    struct {
        uint8_t ucType:5;
        uint8_t ucSn:3;
    } u;

    uint8_t ucData;
} U_DevType;

typedef union {
    struct {
        uint8_t ucEncrypt:4;
        uint8_t ucResv:1;
        uint8_t ucNeed_Ack:2;
        uint8_t ucReq_Ack:1;
    } u;

    uint8_t ucData;
} U_CmdType;

typedef struct {
    uint8_t ucSOF;
    U_LV    uLV;
    uint8_t ucCrc8;

    U_DevType uSender;
    U_DevType uReceiver;

    uint16_t  usSeqNum;
    U_CmdType uCmdType;
    uint8_t   ucCmdSet;
    uint8_t   ucCmdID;
} T_DJIProtocolHeaderStruct;
#pragma pack()

/**
 *  * Structure used for send pack in csdk
 *   * */
typedef struct {
    uint8_t sender;
    uint8_t recver;
    uint8_t cmdtype;
    uint8_t encrypt;
    uint8_t cmdset;
    uint8_t cmdid;
    uint16_t seq;
    uint16_t send_id;
    uint8_t is_response;
} T_CmdAttr;

typedef struct {
    int type;
    int timeout;
} Attr;

typedef struct {
    Attr attr;
    char *ip;
    int port;
} TcpAttr;

typedef struct {
    Attr attr;
    char *dev_path;
    unsigned int baud;
    int flow_ctrl;
    int data_bits;
    int stop_bits;
    char parity;
} UartAttr;
#endif
