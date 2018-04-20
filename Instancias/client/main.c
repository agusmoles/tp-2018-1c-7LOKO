/******** CLIENTE INSTANCIAS *********/

#include "main.h"

void _exit_with_error(int socket, char* mensaje) {
	close(socket);
	log_error(logger, mensaje);
	exit(1);
}

void configurarLogger() {
	logger = log_create("cliente.log", "cliente", 1, LOG_LEVEL_INFO);
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
}

void envioIdentificador(int socket) {
	char* identificador = "1";

	if (send(socket, identificador, strlen(identificador)+1, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el identificador"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio correctamente el identificador"ANSI_COLOR_RESET);
}

void pipeHandler() {
	printf(ANSI_COLOR_BOLDRED"***********************************EL SERVIDOR SE CERRO***********************************\n"ANSI_COLOR_RESET);
	exit(1);
}

int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = pipeHandler;
	sigaction(SIGPIPE, &finalizacion, NULL);


	configurarLogger();
	int socket = conectarSocket();

	reciboHandshake(socket);
	envioIdentificador(socket);
	enviarMensajes(socket);

	close(socket);

	return EXIT_SUCCESS;
}
