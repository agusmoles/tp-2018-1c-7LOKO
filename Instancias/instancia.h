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
#include <math.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>

#define DEBUG 1

char* IP;
char* PUERTO;
char* ALGORITMOREEMPLAZO;
char* PUNTOMONTAJE;
char* NOMBREINSTANCIA;
int INTERVALODUMP;
int TAMANIOENTRADA;
int CANTIDADENTRADAS;
int IDENTIFICADORINSTANCIA;
int STORAGEAPUNTADO;
int DUMP;
int socketCoordinador;
int LEVANTODEDISCO;

sem_t mutexTablaDeEntradas;
sem_t mutexOperaciones;

t_log* logger;
t_config* config;

t_list* tablaEntradas;
t_list* listaStorage;

typedef struct Entrada{
	char* clave;
	int numero;
	int tamanio_valor;
	int largo;
	int cantidadDeNoReferencias;
}entrada_t ;

char* storageFijo;
char* storage;

void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
void inicializarStorage();
int conectarSocket();
void conectarConCoordinador(int socket);
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void levantarClaveDeDisco(char* clave);
void pipeHandler();
void recibirInstruccion(int socket);
int entradasLibres();
void set(char* clave, char* valor);
int reemplazarSegunAlgoritmo(int espaciosNecesarios);
int cantidadValoresAtomicos();
entrada_t* buscarEntradaAtomicaMasGrande(int* posicion);
entrada_t* buscarEntradaMenosReferenciada(int* posicion);
void sumarUnoALasNoReferencias(entrada_t* entrada);
int hayEspaciosNoContiguosPara(int espaciosNecesarios);
int hayEspaciosContiguosPara(int espaciosNecesarios);
void asignarAEntrada(entrada_t* entrada, char* valor, int largo);
void copiarValorAlStorage(entrada_t* entrada, char* valor, int posicion);
void limpiarValores(entrada_t* entrada);
void store(char* clave);
void recibirClave(int socket, header_t* header, char* bufferClave);
void recibirTamanioValor(int socket, int32_t* tamanioValor);
void recibirValor(int socket, int32_t* tamanioValor, char* bufferValor);
entrada_t* buscarEnTablaDeEntradas(char* clave);
char* buscarEnStorage(int entrada);
entrada_t* buscarEntrada(int numeroDeStorage, int* posicion);
void enviarHeaderOperacionOK();
void enviarHeaderOperacionSETFail();
void enviarTamanioValor(int socket, int* tamanioValor);
void enviarValor(int socket, int tamanioValor, char* valor);
void dump();
void mostrarTablaDeEntradas();
void mostrarStorage();
void compactar();
int comparadorNumeroEntrada(entrada_t* entrada, entrada_t* entrada2);
void ordenarTablaDeEntradas();
