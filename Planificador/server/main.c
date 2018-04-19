/******** SERVIDOR PLANIFICADOR *********/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>

#define PUERTO "6666"
#define NUMEROCLIENTES 10

t_log* logger;
fd_set descriptoresLectura;
int fdmax = NUMEROCLIENTES;

void manejoDeClientes(int socket, int* socketCliente);
void aceptarCliente(int socket, int* socketCliente);
void recibirMensaje(int socket, int* socketCliente, int posicion);


void _exit_with_error(int socket, int* socketCliente, char* mensaje) {
	close(socket);

	if (socketCliente != NULL) {
		for(int i=0; i<NUMEROCLIENTES; i++) {
			if (socketCliente[i] != 0) {
				close(socketCliente[i]);					// CIERRO TODOS LOS SOCKETS DE CLIENTES QUE ESTABAN ABIERTOS EN EL ARRAY
			}
		}
	}

	FD_ZERO(&descriptoresLectura);

	log_error(logger, mensaje);
	exit(1);
}

void configurarLogger() {
	logger = log_create("server.log", "server", 1, LOG_LEVEL_INFO);
}

int conectarSocketYReservarPuerto() {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, PUERTO, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE

	int listenSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (listenSocket <= 0) {
		_exit_with_error(listenSocket, NULL, "No se pudo conectar el socket");
	}

	log_info(logger, "Se pudo conectar el socket con éxito");

	if(bind(listenSocket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error(listenSocket, NULL,"No se pudo reservar correctamente el puerto de escucha");
	}

	log_info(logger, "Se pudo reservar correctamente el puerto de escucha del servidor");

	freeaddrinfo(serverInfo);

	return listenSocket;
}


void escuchar(int socket) {
	if(listen(socket, NUMEROCLIENTES)) {
		_exit_with_error(socket, NULL, "No se puede esperar por conexiones");
	}

	log_info(logger, "Se esta escuchando correctamente en el puerto");

}

void manejoDeClientes(int socket, int* socketCliente) {
	FD_ZERO(&descriptoresLectura);			//VACIO EL SET DE DESCRIPTORES
	FD_SET(socket, &descriptoresLectura); //ASIGNO EL SOCKET DE SERVIDOR AL SET

	for (int i=0; i<NUMEROCLIENTES; i++) {
		if(socketCliente[i] != -1) {
			FD_SET(socketCliente[i], &descriptoresLectura);  //SI EL FD ES DISTINTO DE -1 LO AGREGO AL SET
		}
	}

	log_info(logger, "Se esta esperando por conexiones o mensajes de los clientes...");

	switch(select(fdmax+1, &descriptoresLectura, NULL, NULL, NULL)) {
		case 0: log_info(logger, "Expiro el tiempo de espera de conexion o mensaje de los clientes");
				break;

		case -1: _exit_with_error(socket, socketCliente, "Fallo el manejo de clientes");
				break;

		default: for (int i=0; i<NUMEROCLIENTES; i++) {
					if (FD_ISSET(socketCliente[i], &descriptoresLectura)) {
						recibirMensaje(socket, socketCliente, i); //RECIBO EL MENSAJE, DENTRO DE LA FUNCION MANEJO ERRORES
					}
				}

					if (FD_ISSET(socket, &descriptoresLectura)) { //ACA SE TRATA AL SOCKET SERVIDOR, SI DA TRUE ES PORQUE TIENE UN CLIENTE ESPERANDO EN COLA
						aceptarCliente(socket, socketCliente);
					}

				manejoDeClientes(socket, socketCliente);

				break;
	}
}

void aceptarCliente(int socket, int* socketCliente) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	log_info(logger, "Un nuevo cliente requiere acceso al servidor. Procediendo a aceptar..");

	for (int i=0; i<NUMEROCLIENTES; i++) {			//RECORRO TODO EL ARRAY DE CLIENTES
		if (socketCliente[i] == -1) {				//SI ES IGUAL A -1, ES PORQUE TODAVIA NINGUN FILEDESCRIPTOR ESTA EN ESA POSICION

			socketCliente[i] = accept(socket, (struct sockaddr *) &addr, &addrlen);		//ASIGNO FD AL ARRAY

			if (socketCliente[i] > fdmax) {
				fdmax = socketCliente[i];
			}

			if (socketCliente[i] < 0) {
				_exit_with_error(socket, socketCliente, "No se pudo conectar el cliente");		// MANEJO DE ERRORES
			}

			log_info(logger, "Se pudo conectar un cliente %d y esta en la posicion %d del array", socketCliente[i], i);

			break;
		}
	}

}

void recibirMensaje(int socket, int* socketCliente, int posicion) {
	void* buffer = malloc(1024);
	int resultado_recv;

	switch(resultado_recv = recv(socketCliente[posicion], buffer, 1024, MSG_DONTWAIT)) {

		case -1: _exit_with_error(socket, socketCliente, "No se pudo recibir el mensaje del cliente");
				break;

		case 0: log_info(logger, "Se desconecto el cliente %d", posicion);
				close(socketCliente[posicion]); 		//CIERRO EL SOCKET
				socketCliente[posicion] = -1;			//LO VUELVO A SETEAR EN -1 PARA QUE FUTUROS CLIENTES OCUPEN SU LUGAR EN EL ARRAY
				break;

		default: printf("Se recibio el mensaje por parte del cliente %d de %d bytes y dice: %s\n", posicion, resultado_recv, (char*) buffer);
				break;

	}

	free(buffer);
}

int main(void) {
	configurarLogger();
	int socketCliente[NUMEROCLIENTES];
	int listenSocket = conectarSocketYReservarPuerto();

	escuchar(listenSocket);

	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i] = -1;				//INICIALIZO EL ARRAY EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
	}

	manejoDeClientes(listenSocket, socketCliente);

	close(listenSocket);

	return EXIT_SUCCESS;
}
