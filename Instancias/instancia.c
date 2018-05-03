/******** CLIENTE INSTANCIAS *********/

#include "instancia.h"

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
	INTERVALODUMP = config_get_string_value(config, "Intervalo de dump");
	TAMANIOENTRADA = config_get_string_value(config, "Tamanio de Entrada");
	CANTIDADENTRADAS = config_get_string_value(config, "Cantidad de Entradas");
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

struct Entrada{
	char* clave;
	int numero;
	int tamanio;
};

struct Storage {
	int numeroEntrada;
	char* info;
};

void procesarInstruccion(int socket){ // aca se reciben los SETS del coordinador

	char* buffer = malloc(1024);

	if (recv(socket,buffer,1024,MSG_WAITALL)>0){
		compararEnTabla(buffer); // en el buffer hay algo como "SET_11:00_jugador"
		free(buffer);
	}
}

int compararEnTabla(char* instruccion, struct Entrada* tablaDeEntradas, struct Storage* almacenamiento){

	char** args;
	args = string_split(instruccion, "_"); // separo la instruccion para obtener la clave

	char* clave = malloc(strlen(args[1]));
	char* info = malloc(strlen(args[2]));
	strcpy(clave,args[1]);
	strcpy(info,args[2]);

	int tamanio = sizeof(args[2]);

	for(int i=0; i<CANTIDADENTRADAS; i++){
		if(strcmp(clave,tablaDeEntradas[i].clave) == 0)
			guardarEnStorage(clave, info, tablaDeEntradas, almacenamiento);
			free(info);
			free(clave);
			return 0; // retorna 0 cuando encuentra instancia con esa clave
	}


	return asignarInstancia(clave, tamanio, info, tablaDeEntradas); // en caso de no encontrar una entrada con esa clave, procedemos a crear una nueva instancia
}

int asignarInstancia(char* clave, int tamanio, char* info, struct Entrada* tablaDeEntradas, struct Storage* almacenamiento){

	struct Entrada nuevaEntrada;
	strcpy(nuevaEntrada.clave, clave);
	nuevaEntrada.tamanio = tamanio;

	for(int i=0; i<CANTIDADENTRADAS; i++){ // recorro la tabla y meto la entrada en la primer instancia libre
		if(tablaDeEntradas[i].clave != NULL){
			nuevaEntrada.numero = i;
			tablaDeEntradas[i] = nuevaEntrada;
			guardarEnStorage(clave,tamanio, info, tablaDeEntradas, almacenamiento);
			return 1; // retorna 1 cuando puede asignarle una instancia a la entrada en la tabla
		}
	}

	return -1; // retorna -1 cuando la tabla esta llena y no puede asignar una nueva instancia
}

void guardarEnStorage(char* clave, int tamanio, char* info, struct Entrada* tablaDeEntradas, struct Storage* almacenamiento){

	struct Storage storage;
	int numeroEntrada;

	for(int i=0; i<CANTIDADENTRADAS; i++){
		if(strcmp(clave,tablaDeEntradas[i].clave) == 0){
			numeroEntrada = tablaDeEntradas[i].numero;
		}
	}

	strcpy(storage.info, info);
	storage.numeroEntrada = numeroEntrada;

	for(int j=0; j<CANTIDADENTRADAS; j++){
		if(almacenamiento[j].numeroEntrada != NULL){
			almacenamiento[j] = storage;
		}
	}
}

int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = pipeHandler;
	sigaction(SIGPIPE, &finalizacion, NULL);

	struct Entrada tablaDeEntradas[CANTIDADENTRADAS];
	struct Storage almacenamiento[CANTIDADENTRADAS];

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
