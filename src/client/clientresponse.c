#include "../include/clientresponse.h"

void handle_get_ok_status(struct client_request_args arg, uint8_t *buf, uint8_t *combinedlen, uint8_t *numeric_data_array, uint32_t *numeric_response) {
    combinedlen[0] = buf[1];
    combinedlen[1] = buf[2];
   // uint16_t dlen = ntohs(*(uint16_t*)combinedlen); // obtengo el dlen
    switch (arg.target.get_type) {
        case historic_connections:      // recibe uint32 (4 bytes)
        case concurrent_connections:    // recibe uint32 (4 bytes)
        case transferred_bytes:         // recibe uint32 (4 bytes)
            for (int k = 0, j = 3; k < 4; k++) {
                numeric_data_array[k] = buf[j++];
            }
            *numeric_response = ntohl(*(uint32_t*)numeric_data_array);
            if(arg.target.get_type == historic_connections) {
                printf("The amount of historic connections is: %u\n", *numeric_response);
            } else {
                printf("The amount of %s is: %u\n",  arg.target.get_type == concurrent_connections ? "concurrent connections" : "transferred bytes", *numeric_response);
            }
            break;
    default:
        break;
    }
}

void handle_config_ok_status(struct client_request_args arg) {
    switch (arg.target.config_type) {
        case config_maildir:
            printf("The New Path is: '%s'\n", arg.data.path);
            break;
        case add_admin_user:
            printf("The admin: '%s' is now added in the server\n", arg.data.add_admin_user_params.user);
            break;
        case del_admin_user:
            printf("The admin: '%s' is now deleted from the server\n", arg.data.add_admin_user_params.user);
            break;
        case add_pop3_user:
            printf("The new user: '%s' is now added in the server\n", arg.data.add_admin_user_params.user);
            break;
         case del_pop3_user:
            printf("The user: '%s' is now deleted from the server\n", arg.data.add_admin_user_params.user);
            break;
        default:
            //no deberia caer aca
        break;
    }
}

void handle_error_response (struct client_request_args *args, enum monitor_resp_status resp_status) {
    switch (resp_status) {
        case monitor_resp_status_unsuported_version:
            printf("The version of the request you have sent is incorrect!\n");
            break;
        case monitor_resp_status_invalid_type:
            printf("The method of the request you have sent is incorrect!\n");
            break;
        case monitor_resp_status_invalid_target:
            printf("The target of the request you have sent is incorrect!\n");
            break;
        case monitor_resp_status_invalid_data:
            switch (args->target.config_type) {
                 case config_maildir:
                    printf("Error changing the maildir,remember the path has a max lenth!\n");
                    break;
                case add_admin_user:
                    printf("Error adding the admin, admin and token should be alphanumeric or admin already exist!\n");
                    break;
                case del_admin_user:
                    printf("Error deleting the admin, admin name should be alphanumeric, admin does not exist or is default admin!\n");
                    break;
                case add_pop3_user:
                    printf("Error adding the user, user and password should be alphanumeric or user already exist!\n");
                    break;
                case del_pop3_user:
                    printf("Error deleting the user, user name should be alphanumeric\n");
                    break;
                default:
                    printf("The data of the request you have sent is incorrect!\n");
                    break;
                }
            break;
        case monitor_resp_status_error_auth:
            printf("The token of the request you have sent is incorrect!\n");
            break;
        case monitor_resp_status_server_error:
            printf("The server could not resolve your request!\n");
            break;
        default:
            break;
    }
}

void process_response (uint8_t c, struct client_request_args *args, uint8_t *buf, uint8_t *combinedlen, uint8_t *numeric_data_array, uint32_t *numeric_response) {
        if (c == monitor_resp_status_ok) {
            if (args->method == get)
                handle_get_ok_status(*args, buf, combinedlen, numeric_data_array, numeric_response);
          else
                handle_config_ok_status(*args);
        } else {
            handle_error_response (args, c);
        }
}
