#define CMD_MSG_ID 10
#define HEARTBEAT_MSG_ID 11

struct __attribute__((__packed__)) cmd_msg
{
  uint8_t  id;
  uint8_t  set_out_a;
  uint8_t  set_out_b;
  bool  set_out_vbat;
};

struct __attribute__((__packed__)) heartbeat_msg
{
  uint8_t  id;
  bool     is_ok;
  uint16_t cmds_received;
};
