# TP-Protos
 Trabajo Practico Especial de Protocolos de Comunicacion - 1C 2023

## Recursos

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
