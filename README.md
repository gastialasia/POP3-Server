# Servidor POP3 y cliente de monitoreo

Implementación de un servidor para el protocolo POP3 (Post Office Protocol versión 3) programada en C. Cumple con el estándar [RFC1939](https://www.ietf.org/rfc/rfc1939.txt) y algunas extensiones del mismo, especificadas en el [RFC2449](https://datatracker.ietf.org/doc/html/rfc2449)

Permite la lectura y borrado de correos desde un maildir local y conexión de múltiples usuarios de manera simultánea. Puede ser usado por Mail User Agents tales como Mozilla Thunderbird y Outlook para la recepción de correos.

También se incluye un cliente que utiliza un protocolo propio de monitoreo para analizar las métricas del servidor POP3.

## Características principales

El servidor POP3 soporta todos los comandos requeridos en el RFC como CAPA, LIST, STAT, RETR, RSET Y DELE. Con respecto a las extensiones, soporta autenticación con USER y PASS. También soporta pipelining y respuestas multilínea.

El servidor se ejecuta especificando el puerto donde escuchará las conexiones y a partir de ese momento pueden conectarse tanto clientes usuarios como el cliente de monitoreo. Este último es el que permite registrar usuarios y contraseñas válidas para que luego sean autenticados los clientes usuarios.

En caso de que un usuario o el servidor de monitoreo se conecten, el socket pasivo crea uno activo y se establece la conexión TCP. A partir de allí el servidor soporta los comandos enviados por el cliente. Al momento de la desconexión ya sea por un comando QUIT como por un CTRL+C , se liberan los recursos para dicho cliente.
Por defecto, buscará la carpeta de inbox en el directorio current/username/cur, por lo que esta debe estar previamente creada. Los mails dicha carpeta deben tener el formato Internet Message (RFC 5322) y de no tenerlo, el comportamiento del server es inesperado.

Para saber qué esta sucediendo en el servidor, este loguea por salida estándar información sobre cada conexión y desconexión de cliente, y cada log in y log out de usuario.

## Tecnologías utilizadas

Programado enteramente en C y utilizando Sockets

## Recursos adicionales

Toda la documentación del proyecto se encuentra en el directorio documentation

* Informe: Informe.pdf
* RFC del protocolo de monitoreo: MonitorRFC.txt
* Diagrama de arquitectura del proyecto: ArquitecturaDelProyecto.txt

## Compilación

```bash
user@machine:path/ $  make all
```

## Uso del servidor

```bash
user@machine:path/ $ ./pop3 -p [port] -m [maildir]
```
* Por default utiliza el puerto 1100 y el maildir ./directories/

* Nota: el path debe terminar con /

* Al inicializar el servidor, el usuario default es foo:bar

## Uso del cliente

```bash
user@machine:path/ $ ./client [option] token 
```
* Al inicializar el cliente, el token default es 123456789abcdef1
