/******** SERVIDOR PLANIFICADOR *********/

struct Cliente{
	char* nombre;
	int fd;						//ESTRUCTURA PARA RECONOCER A LOS ESI Y DEMAS CLIENTES
};

#include "server-planificador.h"

void _exit_with_error(int socket, struct Cliente* socketCliente, char* mensaje) {
	close(socket);

	if (socketCliente != NULL) {
		for(int i=0; i<NUMEROCLIENTES; i++) {
			if (socketCliente[i].fd != 0) {
				close(socketCliente[i].fd);					// CIERRO TODOS LOS SOCKETS DE CLIENTES QUE ESTABAN ABIERTOS EN EL ARRAY
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

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar el socket con éxito"ANSI_COLOR_RESET);

	if(bind(listenSocket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error(listenSocket, NULL,"No se pudo reservar correctamente el puerto de escucha");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo reservar correctamente el puerto de escucha del servidor"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return listenSocket;
}


void escuchar(int socket) {
	if(listen(socket, NUMEROCLIENTES)) {
		_exit_with_error(socket, NULL, "No se puede esperar por conexiones");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se esta escuchando correctamente en el puerto"ANSI_COLOR_RESET);

}

void manejoDeClientes(int socket, struct Cliente* socketCliente) {
	FD_ZERO(&descriptoresLectura);			//VACIO EL SET DE DESCRIPTORES
	FD_SET(socket, &descriptoresLectura); //ASIGNO EL SOCKET DE SERVIDOR AL SET

	for (int i=0; i<NUMEROCLIENTES; i++) {
		if(socketCliente[i].fd != -1) {
			FD_SET(socketCliente[i].fd, &descriptoresLectura);  //SI EL FD ES DISTINTO DE -1 LO AGREGO AL SET
		}
	}

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Se esta esperando por conexiones o mensajes de los clientes..."ANSI_COLOR_RESET);

	switch(select(fdmax+1, &descriptoresLectura, NULL, NULL, NULL)) {
		case 0: log_info(logger, "Expiro el tiempo de espera de conexion o mensaje de los clientes");
				break;

		case -1: _exit_with_error(socket, socketCliente, "Fallo el manejo de clientes");
				break;

		default: for (int i=0; i<NUMEROCLIENTES; i++) {
					if (FD_ISSET(socketCliente[i].fd, &descriptoresLectura)) {
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

void aceptarCliente(int socket, struct Cliente* socketCliente) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Un nuevo cliente requiere acceso al servidor. Procediendo a aceptar.."ANSI_COLOR_RESET);

	for (int i=0; i<NUMEROCLIENTES; i++) {			//RECORRO TODO EL ARRAY DE CLIENTES
		if (socketCliente[i].fd == -1) {				//SI ES IGUAL A -1, ES PORQUE TODAVIA NINGUN FILEDESCRIPTOR ESTA EN ESA POSICION

			socketCliente[i].fd = accept(socket, (struct sockaddr *) &addr, &addrlen);		//ASIGNO FD AL ARRAY

			if (socketCliente[i].fd < 0) {
				_exit_with_error(socket, socketCliente, "No se pudo conectar el cliente");		// MANEJO DE ERRORES
			}

			if (socketCliente[i].fd > fdmax) {
				fdmax = socketCliente[i].fd;				//SI EL FD ASIGNADO ES MAYOR AL FDMAX (QUE NECESITA EL SELECT() ), LO ASIGNO AL FDMAX
			}

			switch(envioHandshake(socketCliente[i].fd)) {
			case -1: _exit_with_error(socket, socketCliente, "No se pudo enviar el handshake");
					break;
			case -2: _exit_with_error(socket, socketCliente, "No se pudo recibir el handshake");
					break;
			case 0: log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo enviar el handshake correctamente"ANSI_COLOR_RESET);
					break;
			}

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar un cliente %d y esta en la posicion %d del array"ANSI_COLOR_RESET, socketCliente[i].fd, i);

			break;
		}
	}

}

void recibirMensaje(int socket, struct Cliente* socketCliente, int posicion) {
	void* buffer = malloc(1024);
	int resultado_recv;

	switch(resultado_recv = recv(socketCliente[posicion].fd, buffer, 1024, MSG_DONTWAIT)) {

		case -1: _exit_with_error(socket, socketCliente, "No se pudo recibir el mensaje del cliente");
				break;

		case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el cliente %d"ANSI_COLOR_RESET, posicion);
				close(socketCliente[posicion].fd); 		//CIERRO EL SOCKET
				socketCliente[posicion].fd = -1;			//LO VUELVO A SETEAR EN -1 PARA QUE FUTUROS CLIENTES OCUPEN SU LUGAR EN EL ARRAY
				break;

		default: printf(ANSI_COLOR_BOLDGREEN"Se recibio el mensaje por parte del cliente %d de %d bytes y dice: %s\n"ANSI_COLOR_RESET, posicion, resultado_recv, (char*) buffer);
				break;

	}

	free(buffer);
}

int envioHandshake(int socketCliente) {
	char* handshake = "******PLANIFICADOR HANDSHAKE******";

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Enviando handshake..."ANSI_COLOR_RESET);

	switch(send(socketCliente, handshake, strlen(handshake)+1, 0)) {
		case -1: return -1;
				break;
		default: return 0;
				break;
	}
}

void intHandler() {
	printf(ANSI_COLOR_BOLDRED"\n************************************SE INTERRUMPIO EL PROGRAMA************************************\n"ANSI_COLOR_RESET);
	exit(1);
}

int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = intHandler;
	sigaction(SIGINT, &finalizacion, NULL);

	configurarLogger();

	struct Cliente socketCliente[NUMEROCLIENTES];		//ARRAY DE ESTRUCTURA CLIENTE

	int listenSocket = conectarSocketYReservarPuerto();

	escuchar(listenSocket);

	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i].fd = -1;				//INICIALIZO EL ARRAY DE FDS EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
	}

	manejoDeClientes(listenSocket, socketCliente);

	close(listenSocket);

	return EXIT_SUCCESS;
}
