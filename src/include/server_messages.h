#ifndef SERVER_MESSAGES_H
#define SERVER_MESSAGES_H

#define AUTH_CAPA_MSG "+OK\nCAPA\nUSER\nPIPELINING\n.\r\n"
#define TRANS_CAPA_MSG "+OK\nCAPA\nUSER\nPIPELINING\nSTAT\nLIST\nRETR\nDELE\nRSET\nNOOP\n.\n"
#define INVALID_COMMAND_MSG "-ERR Unknown command.\r\n"
#define PASS_NOUSER_MSG "-ERR No username given.\r\n"
#define AUTH_ERROR_MSG "-ERR [AUTH] Authentication failed.\r\n"
#define INVALID_INDEX_MSG "-ERR Invalid message number\n"
#define NO_MSG_MSG "-ERR No such message\n"
#define LIST_FMT "+OK %u messages (%ld octets)\n"
#define LOGIN_MSG "+OK Logged in.\r\n"
#define STAT_FMT "+OK %u %ld\n"
#define DELE_MSG "+OK message deleted\n"
#define AUTH_QUIT_MSG "+OK closing connection\n"
#define ALREADY_DELE_MSG "-ERR message already deleted\n"
#define POSITIVE_MSG "+OK\n"
#define NEGATIVE_MSG "-ERR\n"

#endif