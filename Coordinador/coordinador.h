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

char* PUERTO;
#define NUMEROCLIENTES 20

typedef struct Cliente{
	char nombre[14];
	int fd; 				//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
	int identificadorESI;
	int identificadorInstancia;
}cliente;

struct arg_struct {
	int socket;
	cliente socketCliente;
};

t_log* logger;
t_config* config;
cliente socketCliente[NUMEROCLIENTES];


/*FUNCIONES DE CONEXION */
void _exit_with_error(int socket, char* mensaje);
void configurarLogger();
void crearConfig();
void setearConfigEnVariables();
int conectarSocketYReservarPuerto();
void escuchar(int socket);

/* Asigna nombre a cada cliente particular:  Instancia, ESI, Planificador */
void asignarNombreAlSocketCliente(struct Cliente* socketCliente, char* nombre);


void enviarMensaje(int socketCliente, char* msg);

/* Recibe sentencia del ESI */
void recibirSentenciaESI(void* socketCliente);

void recibirIDdeESI(cliente* cliente);

/* Asigna ID a cada Instancia para identificarlas */
void asignarIDdeInstancia(struct Cliente* socketCliente, int id);

void recibirMensaje(void* argumentos);

/* CREACION DE HILOS PARA CADA CLIENTE */

void crearHiloPlanificador(cliente socketCliente);

void crearHiloInstancia(cliente socketCliente);

void crearHiloESI(cliente* socketCliente);

/* Crea un array con las instancias que se encuentran conectadas y muestra la cantidad*/
void instanciasConectadas();

/* Si no hay instancias conectadas logea el error */
int verificarSiExistenInstanciasConectadas();

/* Distribuye las sentencias a las disitintas instancias */
int seleccionEquitativeLoad();

/*Envia la sentencia a la instancia correspondiente*/
void enviarSentenciaESIaInstancia(int socket, header_t* header, char* clave, char* valor);

/* Maneja todos los clientes que se pueden conectar */
void aceptarCliente(int socket, cliente* socketCliente);
int envioHandshake(int socketCliente) ;

/* Identifica si se conecto ESI, PLANIFICADOR o INSTANCIA */
int reciboIdentificacion(int socketCliente);

void intHandler();

void tratarSegunOperacion(header_t* header, int socket);

void actualizarVectorInstanciasConectadas();


