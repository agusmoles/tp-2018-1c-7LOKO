/*
 * consola.h
 *
 *  Created on: 20 abr. 2018
 *      Author: nacho
 */

#ifndef CONSOLAPLANIFICADOR_SRC_CONSOLA_H_
#define CONSOLAPLANIFICADOR_SRC_CONSOLA_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../../Colores.h"

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
