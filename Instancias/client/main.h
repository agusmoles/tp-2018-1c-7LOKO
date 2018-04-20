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
#define PUERTO "6666"

t_log* logger;
