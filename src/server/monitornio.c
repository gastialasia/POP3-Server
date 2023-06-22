#include "../include/monitor.h"
#include "../include/buffer.h"
#include "../include/pop3nio.h"
#include "../include/selector.h"
#include "../include/email.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


struct monitor_st {
  buffer                *rb,*wb;
  struct monitor        monitor;
  struct monitor_parser parser;
  enum response_code_status status;
};

struct admin_connection{
  int                  client_fd;

  struct monitor_st    request;

  uint8_t raw_buff_a[512],raw_buff_b[512];
  buffer read_buffer,write_buffer;

  struct admin_connection *next;
};
// TODO:@pato descomentar esto cuando destruyamos cosas
// static const unsigned           max_pool = 3;
// static unsigned                 pool_size = 0;
static struct admin_connection *pool = 0;
size_t current_admins = 0;
unsigned pool_size = 0;
const unsigned max_pool =5;




static struct admin_connection* new_admin_connection(int fd){
  struct admin_connection *ret = 0;

  if(pool == NULL){
    ret = malloc(sizeof(*ret));
  }else {
    ret = pool;
    pool = pool->next;
    ret->next = 0;
  }

  if(ret == NULL){
    goto finally;
  }

  memset(ret, 0x00,sizeof(*ret));

  ret->client_fd = fd;

  buffer_init(&ret->read_buffer, 512, ret->raw_buff_a);
  buffer_init(&ret->write_buffer, 512, ret->raw_buff_b);

finally:
  return ret;

}
// TODO: free de mem
//
//      el manejo de monitor_st
//  
//

//agarra el struct admin_connection de la data
#define ATTACHMENT(key) ((struct admin_connection *)(key) ->data)

static void monitor_read (struct selector_key *key);
static void monitor_write (struct selector_key *key);
static void monitor_close (struct selector_key *key);
static void monitor_action(struct selector_key * key, struct monitor_st *d);
static void monitor_end(struct selector_key * key);

static const fd_handler monitor_handler = {
  .handle_read = monitor_read,
  .handle_close = monitor_close,
  .handle_write = monitor_write,
};

static void admin_connection_destroy(struct admin_connection *s) {
    if(s == NULL) return;

    if(pool_size < max_pool) { // agregamos a la pool
        s->next = pool;
        pool    = s;
        pool_size++;
    } else {
        free(s);    // realmente destruye
    }
}

void admin_connection_pool_destroy(void) {
    struct admin_connection *next, *s;
    for(s = pool; s != NULL ; s = next) {
        next = s->next;
        free(s);
    }
}

static void monitor_init(struct admin_connection * admin_connection){
  struct monitor_st *d = &admin_connection->request;
  d->rb                = &(admin_connection->read_buffer);
  d->wb                = &(admin_connection->write_buffer);
  d->parser.monitor    = &d->monitor;
  d->status            = monitor_status_unknown_error;
  monitor_init_parser(&d->parser);
}

void monitor_passive_accept(struct selector_key * key){
  struct admin_connection * admin = NULL;

  const int client_fd = accept(key->fd, NULL, NULL);
 
  if(client_fd == -1){
    goto fail;
  }
  if(selector_fd_set_nio(client_fd) == -1){
    goto fail; 
  }
  admin = new_admin_connection(client_fd);
  if(admin == NULL){
    goto fail;
  }

  if(SELECTOR_SUCCESS != selector_register(key->s, client_fd, &monitor_handler, OP_READ, admin)){
    goto fail;
  }

  monitor_init(admin);
  return;

fail:
  if(client_fd != -1){
    close(client_fd);
  }
// funcion para cerrar la conexion lo quivalente al destroy
   admin_connection_destroy(admin);
}

static void monitor_read(struct selector_key * key){
  struct monitor_st *d = &ATTACHMENT(key)->request;
  
  buffer *b =d->rb;
  uint8_t *ptr;
  size_t count;
  ssize_t n;


  ptr = buffer_write_ptr(b, &count);
  n = recv(key->fd, ptr, count, 0);

  if(n <= 0){

     monitor_end(key);
  
  }else {

    buffer_write_adv(b,n);
    int state = monitor_consume(b,&d->parser);
    
    if(monitor_has_finish(state)){
    
      if(state >= monitor_unknown_error){
      
        if(-1 == monitor_error_handler(&d->parser,d->wb)){
        
          abort();
        }
      }else {

        monitor_action(key,d);
      
      }
      
      selector_set_interest_key(key, OP_WRITE);
    
    }
    
  }

}


static void monitor_write(struct selector_key * key){
  struct monitor_st *d = &ATTACHMENT(key)->request;

  buffer *b = d->wb;
  uint8_t *ptr;
  size_t count;
  ssize_t n;
  

  ptr = buffer_read_ptr(b, &count);
  n = send(key->fd, ptr, count, MSG_NOSIGNAL);

  if(n == -1){
    //finish
    monitor_end(key);
  }else {
    buffer_read_adv(b, n);
      if(!buffer_can_read(b)){
        monitor_end(key);
        //termino de leer, ceramos la con
      }
  }
}

static void monitor_close(struct selector_key * key){
  admin_connection_destroy(ATTACHMENT(key));
}


#define MAX_ADMIN 3
struct admin_info{
  char   user[255];
  char   token[16];
};

struct admin_info all_admin_data[MAX_ADMIN];

static bool check_monitor_admin(char * token){
  for (size_t i =0 ; i < current_admins; i++) {
      if(strncmp(token, all_admin_data[i].token, 15) == 0){
        return true;
      }
  }
  return false;

}
//TODO agregar default admin
int remove_admin(char * username){
  for (size_t i = 0; i < current_admins; i++) {
    if(strcmp(username, all_admin_data[i].user) == 0){
      current_admins--;
      return 0;
    }
  }
  return -1;
}


int add_new_admin(char * user, char * token){
  if(current_admins >= MAX_ADMIN){
    return 1;
  }
  //hacer un define del largo
  if(strlen(token) != 15){
    return -1; //token malo
  }
  //checkeo x si ya existe el username del admin
  for(size_t i =0; i < current_admins; i++){
    if(strcmp(user, all_admin_data[i].user)){
      return -1;
    }
  }
  //TODO decidir el largo del username 
  strncpy(all_admin_data[current_admins].user, user, 5);
  strncpy(all_admin_data[current_admins++].token, token, 16);
  return 0;

}

static void monitor_action(struct selector_key * key, struct monitor_st *d){
  
  uint8_t * data = NULL;
  uint16_t len = 1;
  bool is_number = false;
  int error_type = 0;
  //checkeo si el token es valido
  if(check_monitor_admin(d->parser.monitor->token)){
    d->status = monitor_status_invalid_auth;
    goto end;
  }

    switch (d->parser.monitor->req_type) {
      case monitor_type_data:
          switch (d->parser.monitor->type.data_type) {
            case monitor_data_current: {
              uint32_t current_con = get_current();
              len = sizeof(current_con);
              data = malloc(len);
              *data = current_con;
              is_number = true;
              d->status = monitor_status_success;
              break;
            }
            case monitor_data_historic: {
              uint32_t historic_con = get_historic();
              len = sizeof(historic_con);
              data = malloc(len);
              *data = historic_con;
              is_number = true;
              d->status = monitor_status_success;
              break;
            }
            case monitor_data_tranfer_bytes: {
              uint32_t tranfer_bytes = get_transfer_bytes();
              len = sizeof(tranfer_bytes);
              data = malloc(len);
              *data = tranfer_bytes;
              is_number = true;
              d->status = monitor_status_success;
              break;
            }
            default :{
              d->status = monitor_status_no_such_method;
            }
         }
          break;
      case monitor_type_config:
        switch (d->parser.monitor->type.config_type) {
          case monitor_config_maildir: {
             error_type = change_maildir(d->parser.monitor->data.change_maildir);
             d->status = monitor_status_success;
              break;
          }
          case monitor_config_buf_size: {
             error_type = change_buf_size(d->parser.monitor->data.new_size);
             d->status = monitor_status_success;
              break;
          }
          case monitor_config_add_admin: {
             error_type = add_new_admin(d->parser.monitor->data.admin_to_add.user,d->parser.monitor->data.admin_to_add.token);
             d->status = monitor_status_success;
              break;
          }
          case monitor_config_remove_admin: {
              error_type = remove_admin(d->parser.monitor->data.user_to_del);
              d->status = monitor_status_success;
              break;
          }
          default: {
              d->status = monitor_status_no_such_method;
          }
        }
        break;
      default:{
        d->status =monitor_status_no_such_method;
      }
  }

end:
  if(error_type != 0){
    d->status = monitor_status_invalid_data;
  }

  if(-1 == monitor_response_handler(d->wb,d->status,len,data,is_number)){
    abort();
  }

  free(data);
}

static void monitor_end(struct selector_key *key){
  int fd = ATTACHMENT(key)->client_fd;
  if(fd != -1){
      if(SELECTOR_SUCCESS != selector_unregister_fd(key->s, fd)){
        abort();
      }
      close(fd);
  }

}
