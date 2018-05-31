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
#include "sharedlib.h"

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

struct Entrada{
	char* clave;
	int numero;
	int tamanio;
};

struct Data{
	int numeroEntrada;
	char* info;
};

struct Entrada typedef Entrada;
struct Data typedef Data;

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocket();
void conectarConCoordinador(int socket);
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void pipeHandler();
void recibirInstruccion(int socket);
int procesarInstruccion(char* instruccion);
int asignarInstancia(Entrada nuevaEntrada, Data data);
void guardarEnStorage(Data data);
void recibirClave(int socket, header_t* header, char* bufferClave);
void recibirTamanioValor(int socket, int32_t* tamanioValor);
void recibirValor(int socket, int32_t* tamanioValor, char* bufferValor);






