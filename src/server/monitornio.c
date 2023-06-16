#include "../include/monitor.h"
#include "../include/buffer.h"
#include "../include/pop3nio.h"
#include "../include/selector.h"
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
//      los handlres de monitro ej read,write
//      el fd_handler
//      el init
//      el passive accept
//
//      el manejo de monitor_st
//  
//

//agarra el struct admin_connection de la data
#define ATTACHMENT(key) ((struct admin_connection *)(key) ->data)

static void monitor_read (struct selector_key *key);
static void monitor_write (struct selector_key *key);
static void monitor_close (struct selector_key *key);

static const fd_handler monitor_handler = {
  .handle_read = monitor_read,
  .handle_close = monitor_close,
  .handle_write = monitor_write,
};


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
     //TODO: TERMINO 
  }else {
    
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
  }else {
    buffer_read_adv(b, n);
      if(!buffer_can_read(b)){
        //termino de leer, ceramos la con
      }
  }
}

static void monitor_close(struct selector_key * key){
  //destruimos la conexion?
}

