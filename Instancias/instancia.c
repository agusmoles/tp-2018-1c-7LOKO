/******** CLIENTE INSTANCIAS *********/

#include "instancia.h"

#define CANTIDADENTRADAS 20

static Entrada TABLAENTRADAS[CANTIDADENTRADAS];
static Data STORAGE[CANTIDADENTRADAS];

void _exit_with_error(int socket, char* mensaje) {
	close(socket);
	log_error(logger, mensaje);
	exit(1);
}

void configurarLogger() {
	logger = log_create("instancia.log", "instancia", 1, LOG_LEVEL_INFO);
}

void crearConfig() {
	config = config_create("../cfg");
}

void setearConfigEnVariables() {
	PUERTO = config_get_string_value(config, "Puerto Coordinador");
	IP = config_get_string_value(config, "IP Coordinador");
	ALGORITMOREEMPLAZO = config_get_string_value(config, "Algoritmo de Reemplazo");
	PUNTOMONTAJE = config_get_string_value(config, "Punto de montaje");
	NOMBREINSTANCIA = config_get_string_value(config, "Nombre de la Instancia");
	INTERVALODUMP = config_get_int_value(config, "Intervalo de dump");
	TAMANIOENTRADA = config_get_int_value(config, "Tamanio de Entrada");
}

int conectarSocket() {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(IP, PUERTO, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

	int server_socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (connect(server_socket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error(server_socket, "No se pudo conectar con el servidor");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar con el servidor"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return server_socket;
}

void enviarMensajes(int socket) {
	int enviar = 1;
	char* mensaje;

	printf(ANSI_COLOR_BOLDGREEN"Envia los mensajes que quieras a continuacion ('exit' para salir):\n");

	while (enviar) {
		mensaje = readline("");

		if(strcmp(mensaje,"exit") == 0) {
			free(mensaje);
			break;
		}

		if(send(socket, mensaje, strlen(mensaje)+1, 0) < 0) {
			free(mensaje);
			_exit_with_error(socket, "No se pudo enviar el mensaje");
		}

		free(mensaje);
	}
}

void reciboHandshake(int socket) {
	char* handshake = "******COORDINADOR HANDSHAKE******";
	char* buffer = malloc(strlen(handshake)+1);


	switch (recv(socket, buffer, strlen(handshake)+1, MSG_WAITALL)) {
		case -1: _exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir el handshake"ANSI_COLOR_RESET);
				break;
		case 0:  _exit_with_error(socket, ANSI_COLOR_BOLDRED"Se desconecto el servidor forzosamente"ANSI_COLOR_RESET);
				break;
		default: if (strcmp(handshake, buffer) == 0) {
					log_info(logger, ANSI_COLOR_BOLDGREEN"Se recibio el handshake correctamente"ANSI_COLOR_RESET);
				}
				break;
	}

	free(buffer);
}

void envioIdentificador(int socket) {
	char* identificador = "2";			//INSTANCIA ES 2

	if (send(socket, identificador, strlen(identificador)+1, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el identificador"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio correctamente el identificador"ANSI_COLOR_RESET);
}

void pipeHandler() {
	printf(ANSI_COLOR_BOLDRED"***********************************EL SERVIDOR SE CERRO***********************************\n"ANSI_COLOR_RESET);
	exit(1);
}

void recibirInstruccion(int socket){ // aca se reciben los SETS del coordinador

	char* buffer = malloc(1024);

	if (recv(socket,buffer,1024,MSG_WAITALL)>0){
		procesarInstruccion(buffer); // en el buffer hay algo como "SET_11:00_jugador"
		free(buffer);
	}
}

int procesarInstruccion(char* instruccion){

	char** args;
	args = string_split(instruccion, "_"); // separo la instruccion para obtener la clave

	Entrada entrada;
	strcpy(entrada.clave, args[1]);
	entrada.tamanio = sizeof(args[2]);

	Data data;
	strcpy(data.info, args[2]);

	for(int i=0; i<CANTIDADENTRADAS; i++){	// me fijo si ya existe una instancia con esa clave
		if(strcmp(entrada.clave, TABLAENTRADAS[i].clave) == 0){
			data.numeroEntrada = TABLAENTRADAS[i].numero;
			guardarEnStorage(data);
			return 0; // encuentra instancia con esa clave y guarda la info en storage
		}
	}

	return asignarInstancia(entrada, data); // no encuentra una entrada con esa clave, procedemos a crear una nueva instancia
}

int asignarInstancia(Entrada nuevaEntrada, Data data){

	for(int i=0; i<CANTIDADENTRADAS; i++){ // recorro la tabla y meto la entrada en la primer instancia libre
		if(TABLAENTRADAS[i].clave != NULL){

			nuevaEntrada.numero = i;
			data.numeroEntrada = i;

			TABLAENTRADAS[i] = nuevaEntrada;
			guardarEnStorage(data);

			return 1; // asigna una instancia en la tabla de entradas
		}
	}

	return -1; // la tabla esta llena y no puede asignar una nueva instancia
}

void guardarEnStorage(Data data){

	for(int i=0; i<CANTIDADENTRADAS; i++){
		if(STORAGE[i].numeroEntrada == data.numeroEntrada){
			STORAGE[i] = data;
		}
	}
}

int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = pipeHandler;
	sigaction(SIGPIPE, &finalizacion, NULL);

	configurarLogger();
	crearConfig();
	setearConfigEnVariables();
	int socket = conectarSocket();

	reciboHandshake(socket);
	envioIdentificador(socket);
	enviarMensajes(socket);

	close(socket);

	return EXIT_SUCCESS;
}
