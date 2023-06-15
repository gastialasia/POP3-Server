#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>
#define MAX_NAME 5
#define TOKEN_SIZE 15
#define MAX_PATH 255



enum monitor_state {
  monitor_version,
  monitor_token,
  monitor_method,
  monitor_target,
  monitor_dlen,
  monitor_data,

  //finished monitor_state
  monitor_finish,

  //error
  monitor_unknown_error,
  monitor_unsuported_ver,
  monitor_invalid_token,
  monitor_no_such_method,
  monitor_error_no_such_target,
  monitor_invalid_data,
};

enum response_code_status {
  monitor_status_success          = 0x00,
  monitor_status_unsuported_ver   = 0x01,
  monitor_status_no_such_method   = 0x02,
  monitor_status_no_such_target   = 0x03,
  monitor_status_invalid_data     = 0x04,
  monitor_status_invalid_auth     = 0x05,
  monitor_status_unknown_error    = 0x06,
};

enum monitor_req_type {
  monitor_type_data   = 0x00,
  monitor_type_config = 0x01,
};

enum monitor_data_type {
  monitor_data_historic       = 0x00,
  monitor_data_current        = 0x01,
  monitor_data_tranfer_bytes  = 0x02,
  //agregar mas cosas TODO:
};

enum monitor_config_type{
  monitor_config_maildir      = 0x00,
  monitor_config_buf_size     = 0x01,
  monitor_config_add_admin    = 0x02,
  monitor_config_remove_admin = 0x03,
  //se podrian agregar mas cosas
};


//solo puede existir un request al mismo tiempo
union monitor_type{
  enum monitor_data_type    data_type;
  enum monitor_config_type  config_type;
};

struct add_admin{
  char      user[MAX_NAME];
  char      token[TOKEN_SIZE];
};

union data{
  char       user_to_del[MAX_NAME];//se va a usar para eliminar a un admin
  char             change_maildir[MAX_PATH];
  unsigned         new_size;
  struct add_admin admin_to_add;
};

union data_len {
  uint16_t size;// seria solo numero ej 500
  uint8_t  byteType[2];// seria para poner el tipo osea MB
};

struct monitor {
  char                     token[TOKEN_SIZE];
  enum monitor_req_type    req_type;
  union monitor_type       type;
  uint16_t                 dlen;
  union data               data;
};

struct monitor_parser {
  struct monitor * monitor;
  enum monitor_state state;
  uint16_t len; //bytes que tiene que leer
  uint16_t read; // bytes ya leidos
};





#endif // !MONITOR_H
