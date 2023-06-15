#include "../include/monitor.h"
#include "../include/buffer.h"


struct monitor_st {
  buffer                *rb,*wb;
  struct monitor        monitor;
  struct monitor_parser parser;
  enum response_code_status status;
};


