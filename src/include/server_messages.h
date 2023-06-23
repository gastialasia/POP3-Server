#ifndef SERVER_MESSAGES_H
#define SERVER_MESSAGES_H

#define AUTH_CAPA_MSG "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\n.\r\n"
#define TRANS_CAPA_MSG "+OK\r\nCAPA\r\nUSER\r\nPIPELINING\r\nSTAT\r\nLIST\r\nRETR\r\nDELE\r\nRSET\r\nNOOP\r\n.\r\n"
#define INVALID_COMMAND_MSG "-ERR Unknown command\r\n"
#define PASS_NOUSER_MSG "-ERR No username given\r\n"
#define AUTH_ERROR_MSG "-ERR [AUTH] Authentication failed\r\n"
#define INVALID_INDEX_MSG "-ERR Invalid message number\r\n"
#define NO_MSG_MSG "-ERR No such message\r\n"
#define LIST_FMT "+OK %u messages (%ld octets)\r\n"
#define LOGIN_MSG "+OK Logged in\r\n"
#define STAT_FMT "+OK %u %ld\r\n"
#define DELE_MSG "+OK message deleted\r\n"
#define AUTH_QUIT_MSG "+OK closing connection\r\n"
#define ALREADY_DELE_MSG "-ERR message already deleted\r\n"
#define POSITIVE_MSG "+OK\r\n"
#define NEGATIVE_MSG "-ERR\r\n"

#endif