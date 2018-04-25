#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <signal.h>
#include "../../Colores.h"

#define PUERTO "6666"
#define NUMEROCLIENTES 10

t_log* logger;
fd_set descriptoresLectura;
int fdmax = NUMEROCLIENTES;

void configurarLogger();
int conectarSocketYReservarPuerto();
void escuchar(int socket);
void manejoDeClientes(int socket, struct Cliente* socketCliente);
void aceptarCliente(int socket, struct Cliente* socketCliente);
void recibirMensaje(int socket, struct Cliente* socketCliente, int posicion);
int envioHandshake(int socketCliente);
void _exit_with_error(int socket, struct Cliente* socketCliente, char* mensaje);
