#include <string.h>
#include "../include/monitor.h"


extern void monitor_init_parser(struct monitor_parser *p){
  p->state = monitor_version;
  memset(p->monitor,0, sizeof(*(p->monitor)));
}


