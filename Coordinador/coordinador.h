#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <signal.h>
#include <pthread.h>
#include "../Colores.h"

char* PUERTO;
#define NUMEROCLIENTES 10

t_log* logger;
t_config* config;
fd_set descriptoresLectura;
int fdmax = NUMEROCLIENTES;

void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
void escuchar(int socket);
void manejoDeClientes(int socket, int* socketCliente);
void aceptarCliente(int socket, int* socketCliente);
void recibirMensaje(int socket, int* socketCliente, int posicion);
int envioHandshake(int socketCliente);
int reciboIdentificacion(int socketCliente);
