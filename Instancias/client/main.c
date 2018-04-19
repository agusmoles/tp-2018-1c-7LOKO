/******** CLIENTE *********/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <readline/readline.h> // Para usar readline

#define IP "127.0.0.1"
#define PUERTO "6666"

t_log* logger;

void configurarLogger() {
	logger = log_create("cliente.log", "cliente", 1, LOG_LEVEL_INFO);
}


void _exit_with_error(int socket, char* mensaje) {
	close(socket);
	log_error(logger, mensaje);
	exit(1);
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

	log_info(logger, "Se pudo conectar con el servidor");

	freeaddrinfo(serverInfo);

	return server_socket;
}

void enviarMensajes(int socket) {
	int enviar = 1;
	char* mensaje = malloc(1024);

	printf("Envia los mensajes que quieras a continuacion ('exit' para salir):\n");

	while (enviar) {
		mensaje = readline("");

		if(send(socket, mensaje, strlen(mensaje)+1, 0) < 0) {
			free(mensaje);
			_exit_with_error(socket, "No se pudo enviar el mensaje");
		}

		if(strcmp(mensaje,"exit") == 0) {
			free(mensaje);
			enviar = 0;
		}

		free(mensaje);
	}
}

int main(void) {
	configurarLogger();
	int socket = conectarSocket();
	enviarMensajes(socket);

	close(socket);

	return EXIT_SUCCESS;
}
