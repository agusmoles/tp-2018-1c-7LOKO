#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <readline/readline.h> // Para usar readline
#include <signal.h>
#include "../../Colores.h"

#define IP "127.0.0.1"
#define PUERTO "6667"

t_log* logger;

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
int conectarSocket();
void enviarMensajes(int socket);
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void pipeHandler();
