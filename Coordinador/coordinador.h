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
#include <signal.h>
#include <pthread.h>
#include "../Colores.h"
#include "sharedlib.h"
#include <semaphore.h>

char* PUERTO;
char* ALGORITMODEDISTRIBUCION;
int RETARDO;
#define NUMEROCLIENTES 200
#define CANTIDADCLAVES 200
#define TAMANIOMAXIMOCLAVE 40

typedef struct Cliente{
	char nombre[14];
	int fd; 				//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
	int identificadorESI;
	int identificadorInstancia;
	char primeraLetra;
	char ultimaLetra;
}cliente_t;

typedef struct arg_struct {
	int socket;
	cliente_t socketCliente;
}arg_t;

typedef struct arg_esi {
	int socketPlanificador;
	cliente_t* socketCliente;
}arg_esi_t;

typedef struct clave{
	char* clave;
	int instancia;
}clave_t;

typedef struct instancia{
	int identificadorInstancia;
	int conectada;
}instancia_t;

t_log* logger;
t_log* logOperaciones;
t_config* config;
cliente_t socketCliente[NUMEROCLIENTES];
clave_t clavesExistentes[CANTIDADCLAVES];
sem_t semaforo_planificador;
sem_t semaforo_instancia;
sem_t semaforo_planificadorOK;
sem_t semaforo_instanciaOK;
sem_t mutexEsiEjecutando;
sem_t mutexVectorInstanciasConectadas;

int listenSocketStatus;

/*FUNCIONES DE CONEXION */
void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
void escuchar(int socket);

/* Asigna nombre a cada cliente particular:  Instancia, ESI, Planificador */
void asignarNombreAlSocketCliente(cliente_t* socketCliente, char* nombre);

void enviarMensaje(int socketCliente, char* msg);

/* Recibe sentencia del ESI */
void recibirSentenciaESI(void* socketCliente);

void recibirIDdeESI(cliente_t* cliente);

/* Asigna ID a cada Instancia para identificarlas */
void asignarIDdeInstancia(cliente_t* socketCliente, int id);

void recibirMensaje(void* argumentos);

/* CREACION DE HILOS PARA CADA CLIENTE */

void crearHiloPlanificador(cliente_t socketCliente);

void crearHiloInstancia(cliente_t socketCliente);

void crearHiloESI(cliente_t* socketCliente, int socketPlanificador);

void crearHiloStatus();

int conectarSocketYReservarPuertoDeStatus();

void recibirMensaje_Instancias(void* argumentos);

void recibirMensaje_Planificador(void* argumentos);

void recibirMensajeStatus();

/* Crea un array con las instancias que se encuentran conectadas y muestra la cantidad*/
void instanciasConectadas();

/* Si no hay instancias conectadas logea el error */
int verificarSiExistenInstanciasConectadas();

/* Distribuye las sentencias a las disitintas instancias */
int seleccionEquitativeLoad();

/*Envia la sentencia a la instancia correspondiente*/
void enviarSetInstancia(int socket, header_t* header, char* clave, char* valor);
void enviarStoreInstancia(int socket, header_t* header, char* clave);

/* Envia la sentencia al planificador */
void enviarSentenciaAPlanificador(int socket, header_t* header, char* clave, int idESI);

/* Maneja todos los clientes que se pueden conectar */
void aceptarCliente(int socket, cliente_t* socketCliente);
int envioHandshake(int socketCliente) ;

/* Identifica si se conecto ESI, PLANIFICADOR o INSTANCIA */
int reciboIdentificacion(int socketCliente);

void intHandler();

void enviarSentenciaAPlanificador(int socket, header_t* header, char* clave, int idESI);

void tratarSegunOperacion(header_t* header, cliente_t* socket, int socketPlanificador);

void actualizarVectorInstanciasConectadas();

/* Envios header, clave, valor, idESI */
void enviarHeader(int socket, header_t* header);

void enviarClave(int socket, char* clave);

void enviarValor(int socket, char* valor);

void enviarIDEsi(int socket, int idESI);

int verificarInstanciaConectada(int idInstancia);

int buscarSocketPlanificador();
int buscarSocketESI();
int buscarSocketInstancia(int idInstancia);

int verificarClaveTomada(int socket);

cliente_t* buscarESI(int* IDESI);

int buscarInstanciaEncargada(char* clave);

int existeIdInstancia(int idInstancia);

void notificarAbortoAPlanificador(int socket, header_t* header, int idEsi);

void desconectarInstancia(int idInstancia);
int buscarInstanciaDesconectada();
int cantidadinstanciasIDsUsados();
int hayInstanciasDesconectadas();
void conectarInstancia(int idInstancia);
void agregarInstanciaAVectorIDs(int identificadorInstancia);

void enviarClavesInstancia(cliente_t socketInstancia);
int cantidadClavesDeInstancia(int idInstancia);

void eliminarClaveDeTabla(char* clave);
