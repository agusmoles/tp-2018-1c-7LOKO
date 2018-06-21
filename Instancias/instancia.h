#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <readline/readline.h> // Para usar readline
#include <signal.h>
#include <commons/collections/list.h>
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

t_list* tablaEntradas;
char* storage[CANTIDADENTRADAS];

char* claveBuscada;

typedef struct Entrada{
	char* clave;
	int numero;
	int tamanio_valor;
}entrada_t ;

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
void set(char* clave, char* valor);
void store(char* clave);
void recibirClave(int socket, header_t* header, char* bufferClave);
void recibirTamanioValor(int socket, int32_t* tamanioValor);
void recibirValor(int socket, int32_t* tamanioValor, char* bufferValor);
entrada_t* buscarEnTablaDeEntradas(char* clave);
data_t* buscarEnStorage(int entrada);






