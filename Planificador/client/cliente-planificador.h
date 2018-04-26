#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <readline/readline.h> // Para usar readline
#include <signal.h>
#include "../../Colores.h"

char* IP;
char* PUERTO;

t_log* logger;
t_config* config;

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocket();
void enviarMensajes(int socket);
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void pipeHandler();
