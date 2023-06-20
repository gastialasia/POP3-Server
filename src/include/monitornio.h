#ifndef MONITORNIO_H
#define MONITORNIO_H

//Handler del socket pasivo
void monitor_passive_accept(struct selector_key * key);

// Funcion que se usa para agregar usuarios al rfc de monitoreo
// ret 0 si salio todo bien
// ret 1 si se alcanzo el limite de admins
// ret -1 si ya existe el admin
int add_new_admin(struct add_admin new);
//libera las pools
void admin_connection_pool_destroy(void);
#endif
