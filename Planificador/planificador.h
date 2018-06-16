#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include "../Colores.h"
#include "sharedlib.h"

char* PUERTOPLANIFICADOR;
char* PUERTOCOORDINADOR;
char* algoritmoPlanificacion;
int listenSocket;
double alfaPlanificacion;
int estimacionInicial;
#define NUMEROCLIENTES 20
#define DEBUG 1



t_log* logger;
t_config* config;
t_list* ejecutando;
sem_t mutexEjecutando;
sem_t esisListos;
fd_set descriptoresLectura;
cliente socketCliente[NUMEROCLIENTES];		//ARRAY DE ESTRUCTURA CLIENTE

void configurarLogger();
void crearConfig();
void crearDiccionarioDeClaves();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
int conectarSocketCoordinador();
void reciboHandshake(int socket);
void envioIdentificador(int socket);
void conectarConCoordinador();
void abortarESI(int* IDESI);
void informarAlCoordinador(int socketCoordinador, int operacion);
void escuchar(int socket);
int envioHandshake(int socketCliente);
int envioIDDeESI(int socketCliente, int identificador);
void manejoDeClientes();
void aceptarCliente(int socket);
void planificar();
void recibirHeader(int socket, header_t* header);
void recibirClave(int socket, int tamanioClave, char* clave);
void recibirIDDeESI(int socket, int* ID);
void recibirMensaje(cliente* ESI);
void ordenarProximoAEjecutar();
cliente* getPrimerESIListo();
void ordenarColaDeListosPorSJF();
void verificarDesalojoPorSJF(cliente* ESIEjecutando);
void ordenarColaDeListosPorHRRN();
void enviarOrdenDeEjecucion(cliente* esiProximoAEjecutar, char* ordenEjecucion);
int comparadorRafaga(cliente* cliente, struct Cliente* cliente2);
int comparadorResponseRatio(cliente* cliente, struct Cliente* cliente2);
void sumarUnoAlWaitingTime(cliente* cliente);
void calcularResponseRatio(cliente* cliente);
