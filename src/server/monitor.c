#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "../include/monitor.h"
#include "../include/buffer.h"


#define IS_ALNUM(x) (x>='a' && (x) <= 'z') || (x>='A' && x <= 'Z') || (x>='0' && x <= '9')
#define IS_NUM(x) (x>='0' && x <= '9')

static uint8_t combinedlen[2] = {0};
static size_t username_len = 0;

extern void monitor_init_parser(struct monitor_parser *p){
  p->state = monitor_version;
  memset(p->monitor,0, sizeof(*(p->monitor)));
}

extern bool monitor_has_finish(enum monitor_state st){
  return st >= monitor_finish;
}

enum monitor_state monitor_parser_feed(struct monitor_parser *p, uint8_t c);



extern enum monitor_state monitor_consume(buffer *b, struct monitor_parser *p){
  enum monitor_state st = p->state;

  while (buffer_can_read(b)) {
    uint8_t c = buffer_read(b);
    st = monitor_parser_feed(p,c);
    if(monitor_has_finish(st)){
      break;
    }
  }
  return st;
}

enum monitor_state parse_version(struct monitor_parser *p, uint8_t c){
  enum monitor_state ret;
  switch (c) {
    case 1:
     p->read = 0;
     p->len = TOKEN_SIZE;
     ret = monitor_token;
     break;
    default:
     ret = monitor_unsuported_ver;
     break;
  }
  return ret;
}

enum monitor_state parse_token(struct monitor_parser *p, uint8_t c){
  p->monitor->token[p->read++] = c;
  if(p->read == TOKEN_SIZE){
    return monitor_method;
  }
  return monitor_token;

}

enum monitor_state parse_type(struct monitor_parser *p,uint8_t c){
  enum monitor_state ret;
  p->monitor->req_type = c;
  if(p->monitor->req_type == monitor_type_data || p->monitor->req_type == monitor_type_config){
    ret = monitor_target;
  }else {
    ret = monitor_no_such_method;
  }
  return ret;
}

enum monitor_state parser_target(struct monitor_parser *p,uint8_t c){
  enum monitor_state ret;

  switch (p->monitor->req_type) {
    case monitor_type_data:{
      switch (c) {
        case monitor_data_current:
        case monitor_data_historic:
        case monitor_data_tranfer_bytes:
          p->monitor->type.data_type = c;
          ret = monitor_finish;
        break;
        default:
          ret = monitor_error_no_such_target;
        break;
      }
      break;

    }
    case monitor_type_config:{
      switch (c) {
        case monitor_config_maildir:
        case monitor_config_buf_size:
        case monitor_config_add_admin:
        case monitor_config_remove_admin:
          p->monitor->type.config_type = c;
          p->read = 0;
          p->len = 2; //bytes
          ret = monitor_dlen;
          break;
        default:
          ret = monitor_error_no_such_target;
          break;
      }
      break;
    }
    default:
    // no deberia ser posible llegar aca
    break;
  
  }
  return ret;
}

void reset_len(struct monitor_parser *p, uint16_t len) {
    p->read = 0;
    p->len = len;
}


enum monitor_state parse_dlen(struct monitor_parser * p , uint8_t c){
   enum monitor_state ret;

    combinedlen[p->read++] = c;
    ret = monitor_dlen;

    if (p->read >= p->len) {
        p->monitor->dlen = ntohs(*(uint16_t*)combinedlen); // Para evitar problemas de endianness armo el uint16 de dlen segun el endianness del sistema. (Suponiendo que me los manda en network order "bigendean")
        switch (p->monitor->type.config_type) {
            case monitor_config_maildir:
                reset_len(p, p->monitor->dlen);
                ret = monitor_data;
                break;
            case monitor_config_add_admin:
                reset_len(p, p->monitor->dlen);
                ret = monitor_data;
                break;
            case monitor_config_buf_size:
                reset_len(p, p->monitor->dlen);
                ret = monitor_data;
                break;
            case monitor_config_remove_admin:
                reset_len(p, p->monitor->dlen);
                ret = monitor_data;
                break;
            default:
                ret = monitor_unknown_error;
                break;
        }
    }

    return ret;
}


enum monitor_state parse_data(struct monitor_parser *p, uint8_t c){
   enum monitor_state ret;

    switch(p->monitor->type.config_type) {
        case monitor_config_add_admin:
            // Si el primer caracter es 0 directamente tiro error ya que el usuario no puede ser vacio
            if (p->read == 0 && c == 0) {
                ret = monitor_invalid_data;
                break;
            }

            if (IS_ALNUM(c)) {
                if (p->finish_user == 0) {
                    p->monitor->data.admin_to_add.user[p->read++] = c;
                } else {
                    p->monitor->data.admin_to_add.token[p->read - username_len] = c;
                    p->read++;
                }
                ret = monitor_data;
            } else if (c == 0 && p->finish_user == 0) { // primer separador \0 pongo el null terminated en el username
                p->monitor->data.admin_to_add.user[p->read++] = c;
                p->finish_user = 1;
                username_len = p->read;
                ret = monitor_data;
            } else { // Si no es alfanumerico ni fue el primer 0 separador entonces no es un dato valido
                ret = monitor_invalid_data;
                break;
            }

            if (p->read >= p->len) {
                p->monitor->data.admin_to_add.token[p->read] = 0; // null terminated para password
                ret = monitor_finish;
                p->finish_user = 0;
                break;
            }

            break;

        case monitor_config_remove_admin:
            if (IS_ALNUM(c)) {
                p->monitor->data.user_to_del[p->read++] = c;
                ret = monitor_data;
            } else {
                ret = monitor_invalid_data;
                break;
            }

            if (p->read >= p->len) {
                p->monitor->data.user_to_del[p->read] = 0; // null terminated para username
                ret = monitor_finish;
                break;
            }

            break;
        case monitor_config_maildir:
            if(IS_ALNUM(c) || c == '/'){
              p->monitor->data.change_maildir[p->read++] = c;
              ret = monitor_data;
            }else{
              ret = monitor_invalid_data;
              break;
            }

            if(p->read >= p->len){
              p->monitor->data.change_maildir[p->read] = 0;
              ret = monitor_finish;
              break;
            }

            break;
        case monitor_config_buf_size:
            if(IS_NUM(c)){
              p->monitor->data.new_size[p->read++] = c;
              ret = monitor_data;
            }else {
              ret = monitor_invalid_data;
            }

            if(p->read >= p->len){
              p->monitor->data.user_to_del[p->read] = 0;
              ret = monitor_finish;
            }
          break;
    }

    return ret;
}

enum monitor_state monitor_parser_feed(struct monitor_parser *p, uint8_t c){
  enum monitor_state ret;

  switch (p->state) {
    case monitor_version:{
      ret = parse_version(p,c);
      break;
    }
    case monitor_token:{
      ret = parse_token(p,c);   
      break;
    }
    case monitor_method:{
      ret = parse_type(p,c);
      break;
     }
    case monitor_target:{
      ret = parser_target(p,c);
      break;
    }
    case monitor_dlen:{
      ret = parse_dlen(p,c);
      break;
    }
    case monitor_data:{
      ret = parse_data(p,c);
      break;
    }
    case monitor_finish:
    case monitor_unknown_error:
    case monitor_unsuported_ver:
    case monitor_invalid_token:
    case monitor_no_such_method:
    case monitor_error_no_such_target:
    case monitor_invalid_data:
      ret = p->state;
      break;
    default:{
      ret = monitor_unknown_error;
    }
  
  }
  p->state = ret;
  return p->state;
}

extern int monitor_error_handler(struct monitor_parser *p, buffer *b) {
    enum monitor_state st = p->state;

    size_t n;
    buffer_write_ptr(b, &n);

    if (n < 4)
        return -1;

    // STATUS
    switch(st) {
        case monitor_unsuported_ver:
            buffer_write(b, monitor_status_unsuported_ver);
            break;
        case monitor_invalid_token:
            buffer_write(b, monitor_status_invalid_auth);
            break;
        case monitor_no_such_method:
            buffer_write(b, monitor_status_no_such_method);
            break;
        case monitor_error_no_such_target:
            buffer_write(b, monitor_status_no_such_target);
            break;
        case monitor_invalid_data:
            buffer_write(b, monitor_status_invalid_data);
            break;
        default:
            buffer_write(b, monitor_status_unknown_error);
            break;
    }


    union data_len datalen;
    datalen.size = htons(1);

    // DLEN
    buffer_write(b, datalen.byteType[0]);
    buffer_write(b, datalen.byteType[1]);

    // DATA
    buffer_write(b, 0);

    return 4;
}
int monitor_response_handler(buffer *b,const enum response_code_status status, uint16_t dlen, void *data,bool is_number) {
    // llenar status y dlen primero, checkeando el espacio que hay en el buffer (si te quedas sin espacio en el buffer retornas -1)
    size_t n;
    buffer_write_ptr(b, &n);

    if (n < (size_t) dlen + 3)
        return -1;


    union data_len response_len;
    response_len.size = htons(dlen);

    buffer_write(b, status);
    //mando dlen
    buffer_write(b, response_len.byteType[0]);
    buffer_write(b, response_len.byteType[1]);

    if (is_number) {
        uint8_t numeric_response[4];

        uint32_t number = htonl(*((uint32_t*)data));
        printf("%d\n",number);
        memcpy(numeric_response, &number, sizeof(uint32_t));

        for (int i = 0; i < 4; i++) {
            buffer_write(b, numeric_response[i]);
        }
    } else {
        uint8_t *databytes = (uint8_t *) data;
        if (databytes == NULL) {
            buffer_write(b, 0);
        } else {
            for (uint16_t i = 0; i < dlen; i++) {
                buffer_write(b, databytes[i]);
            }
        }
    }

    return dlen + 3;
}
