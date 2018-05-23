#ifndef CONSOLAPLANIFICADOR_SRC_CONSOLA_H_
#define CONSOLAPLANIFICADOR_SRC_CONSOLA_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <semaphore.h>
#include "../Colores.h"

#define IP "127.0.0.1"
#define PUERTO "6666"

t_log* logger;
sem_t pausado;

int ejecutar_consola();
int com_pausar(char **args);
int com_continuar(char **args);
int com_bloquear(char **args);
int com_desbloquear(char **args);
int com_listar(char **args);
int com_kill(char **args);
int com_status(char **args);
int com_deadlock(char **args);
int com_man(char **args);

int ejecutar_linea(char **args);

void liberar_parametros(char **args);

#endif /* CONSOLAPLANIFICADOR_SRC_CONSOLA_H_ */
