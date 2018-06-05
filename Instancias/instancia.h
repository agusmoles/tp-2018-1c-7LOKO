#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <readline/readline.h> // Para usar readline
#include <signal.h>
#include "../Colores.h"

char* IP;
char* PUERTO;
char* ALGORITMOREEMPLAZO;
char* PUNTOMONTAJE;
char* NOMBREINSTANCIA;
int INTERVALODUMP;
int TAMANIOENTRADA;
int CANTIDADENTRADAS;

t_log* logger;
t_config* config;

struct Instancia{
	Entrada TABLAENTRADAS[CANTIDADENTRADAS];
	Data STORAGE[CANTIDADENTRADAS];
};

struct Entrada{
	char* clave;
	int numero;
	int tamanio;
};

struct Data{
	int numeroEntrada;
	char* info;
};

struct Instancia typedef Instancia;
struct Entrada typedef Entrada;
struct Data typedef Data;

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocket();
void enviarMensajes(int socket);
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void pipeHandler();
void recibirInstruccion(int socket);
int procesarInstruccion(char*);
int asignarInstancia(Entrada, Data);
void guardarEnStorage(Data);












