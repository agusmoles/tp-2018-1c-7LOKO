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
void manejoDeClientes(int socket, int* socketCliente);
void aceptarCliente(int socket, int* socketCliente);
void recibirMensaje(int socket, int* socketCliente, int posicion);
int envioHandshake(int socketCliente);
int reciboIdentificacion(int socketCliente);
