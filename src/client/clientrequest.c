#include <string.h>
#include "../include/clientargs.h"
#include "../include/clientrequest.h"
static void serialize_config_data(struct client_request_args *args, char *buffer);

void serialize_request(struct client_request_args *args, char *token, char *buffer)
{
    buffer[FIELD_VERSION_INDEX] = PROGRAM_VERSION;
    memcpy(FIELD_TOKEN(buffer), token, TOKEN_SIZE);

    buffer[FIELD_METHOD_INDEX] = args->method;

    // Sending in network order
    uint16_t dlen = htons(args->dlen);
    memcpy(FIELD_DLEN(buffer), &dlen, sizeof(uint16_t));

    switch (args->method)
    {
    case get:
        memcpy(FIELD_TARGET(buffer), &args->target.get_type, sizeof(uint8_t));
        memcpy(FIELD_DATA(buffer), &args->data.optional_data, sizeof(uint8_t)); // data = 0 in get case
        break;
    case config:
        memcpy(FIELD_TARGET(buffer), &args->target.get_type, sizeof(uint8_t));
        serialize_config_data(args, buffer);
        break;
    default:
        // should not get here
        break;
    }
}

static void serialize_config_data(struct client_request_args *args, char *buffer)
{
    size_t username_len;
    size_t extra_param_len;

    switch (args->target.config_type)
    {
    case add_admin_user:
        username_len = strlen(args->data.add_admin_user_params.user);
        memcpy(FIELD_DATA(buffer), args->data.add_admin_user_params.user, username_len);

        buffer[FIELD_DATA_INDEX + username_len] = args->data.add_admin_user_params.finish_user;

        extra_param_len = strlen(args->data.add_admin_user_params.token);
        memcpy(FIELD_DATA(buffer) + username_len + 1, args->data.add_admin_user_params.token, extra_param_len);

        break;
    case add_pop3_user:
        username_len = strlen(args->data.add_pop3_user_params.user);
        memcpy(FIELD_DATA(buffer), args->data.add_pop3_user_params.user, username_len);

        buffer[FIELD_DATA_INDEX + username_len] = args->data.add_pop3_user_params.separator;

        extra_param_len = strlen(args->data.add_pop3_user_params.pass);
        memcpy(FIELD_DATA(buffer) + username_len + 1, args->data.add_pop3_user_params.pass, extra_param_len);

        break;
    case del_pop3_user:
    case del_admin_user:
        memcpy(FIELD_DATA(buffer), args->data.user, args->dlen);
        break;
    case config_maildir:
        extra_param_len = strlen(args->data.path);
        memcpy(FIELD_DATA(buffer), args->data.path, extra_param_len);
        break;
    default:
        // no deberia caer nunca aca
        break;
    }
}
