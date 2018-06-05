/******** CLIENTE INSTANCIAS *********/

#include "instancia.h"

#define CANTIDADENTRADAS 20

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

void conectarConCoordinador(int socket) {
	int flag = 1;
	fd_set descriptorCoordinador;
	int fdmax;

	while(flag) {
		FD_ZERO(&descriptorCoordinador);
		FD_SET(socket, &descriptorCoordinador);
		fdmax = socket;

		select(fdmax + 1, &descriptorCoordinador, NULL, NULL, NULL);

		if (FD_ISSET(socket, &descriptorCoordinador)) {
			recibirInstruccion(socket);
		}
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
	header_t* buffer_header = malloc(sizeof(header_t));
	char* bufferClave;
	char* bufferValor;
	int32_t* tamanioValor;

	if(recv(socket, buffer_header, sizeof(header_t), MSG_WAITALL)< 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir el Header"ANSI_COLOR_RESET);
	}

	bufferClave = malloc(buffer_header->tamanioClave);
	tamanioValor = malloc(sizeof(int32_t));

	if(buffer_header->codigoOperacion == 1){  /* SET */
		recibirClave(socket, buffer_header, bufferClave);
		recibirTamanioValor(socket, tamanioValor);

		bufferValor = malloc(*tamanioValor);

		recibirValor(socket, tamanioValor, bufferValor);
	}

	procesarInstruccion(*bufferClave, *tamanioValor, *bufferValor);

	free(tamanioValor);
	free(bufferClave);
	free(bufferValor);
}

int procesarInstruccion(char* clave, int tamanioValor, char* valor){

	Entrada entrada;
	strcpy(entrada.clave, clave);
	entrada.tamanio = tamanioValor;

	Data data;
	strcpy(data->info, valor);

	// ADAPTAR A PARTIR DE ACA PARA QUE FUNCIONE CON LISTA. (VER COMMONS "<commons/collections/list.h>")
	for(int i=0; i<CANTIDADENTRADAS; i++){	// me fijo si ya existe una entrada con esa clave
		if(strcmp(entrada.clave, TABLAENTRADAS[i].clave) == 0){
			data.numeroEntrada = i;
			guardarEnStorage(data);
			return 0; // encuentra entrada con esa clave y guarda la info en storage
		}
	}

}


void guardarEnStorage(Data data){
	list_add(STORAGE, data); // falta contemplar para cuando ya se hay una clave para
}

void recibirClave(int socket, header_t* header, char* bufferClave){
	if (recv(socket, bufferClave, header->tamanioClave, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se recibio la clave"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se recibio la clave %s"ANSI_COLOR_RESET, bufferClave);
}

void recibirTamanioValor(int socket, int32_t* tamanioValor){
	if (recv(socket, tamanioValor, sizeof(int32_t), MSG_WAITALL) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se recibio el tamanio del valor"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se recibio el tamaño del valor de la clave (%d bytes)"ANSI_COLOR_RESET, *tamanioValor);

}

void recibirValor(int socket, int32_t* tamanioValor, char* bufferValor){
	if (recv(socket, bufferValor, (*tamanioValor), MSG_WAITALL) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se recibio el valor de la clave"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se recibio el valor de la clave %s"ANSI_COLOR_RESET, bufferValor);
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
	conectarConCoordinador(socket);

	close(socket);

	return EXIT_SUCCESS;
}
