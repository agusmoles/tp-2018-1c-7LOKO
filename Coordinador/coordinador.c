/******** SERVIDOR COORDINADOR *********/

#include "coordinador.h"

void _exit_with_error(int socket, char* mensaje) {
	close(socket);

	for(int i=0; i<NUMEROCLIENTES; i++) {
		if (socketCliente[i].fd != -1) {
			close(socketCliente[i].fd);	// CIERRO TODOS LOS SOCKETS DE CLIENTES QUE ESTABAN ABIERTOS EN EL ARRAY
		}
	}

	log_error(logger, mensaje);
	exit(1);
}

void configurarLogger() {
	logger = log_create("coordinador.log", "coordinador", 1, LOG_LEVEL_INFO);
}

void crearConfig() {
	config = config_create("../cfg");
}

void setearConfigEnVariables() {
	PUERTO = config_get_string_value(config, "Puerto Coordinador");
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
		_exit_with_error(listenSocket, "No se pudo conectar el socket");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar el socket con éxito"ANSI_COLOR_RESET);

	if(bind(listenSocket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error(listenSocket, "No se pudo reservar correctamente el puerto de escucha");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo reservar correctamente el puerto de escucha del servidor"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return listenSocket;
}

void escuchar(int socket) {
	if(listen(socket, NUMEROCLIENTES)) {
		_exit_with_error(socket, "No se puede esperar por conexiones");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se esta escuchando correctamente en el puerto"ANSI_COLOR_RESET);

}

void asignarNombreAlSocketCliente(struct Cliente* socketCliente, char* nombre) {
	strcpy(socketCliente->nombre, nombre);
}

void crearHiloPlanificador(struct Cliente socketCliente){
	pthread_t threadPlanificador;

	struct arg_struct* args = malloc(sizeof(socketCliente)+sizeof(int));
	args->socket=socket;
	args->socketCliente.fd = socketCliente.fd;
	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if(pthread_create(&threadPlanificador, NULL, (void *) recibirMensaje, args)!=0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo planificador"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo planificador"ANSI_COLOR_RESET);
	pthread_detach(threadPlanificador);
}

void crearHiloInstancia(struct Cliente socketCliente){
	pthread_t threadInstancia;

	struct arg_struct* args = malloc(sizeof(socketCliente)+sizeof(int));
	args->socket=socket;
	args->socketCliente.fd = socketCliente.fd;
	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if(pthread_create(&threadInstancia, NULL, (void *) recibirMensaje, args)!=0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo instancia"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo instancia"ANSI_COLOR_RESET);
	pthread_detach(threadInstancia);
}

void enviarMensaje(int socketCliente, char* msg){
	if(send(socketCliente,msg, strlen(msg)+1,0)<0){
		_exit_with_error(socketCliente, "No se pudo recibir la sentencia");
	}

}

void recibirSentenciaESI(int socketCliente){
	char* buffer = malloc(1024);
	int flag = 1;
	fd_set descriptoresLectura;

	enviarMensaje(socketCliente, "OPOK");

	while(flag) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(socketCliente, &descriptoresLectura);

		select(socketCliente + 1 , &descriptoresLectura, NULL, NULL, NULL);

		if (FD_ISSET(socketCliente, &descriptoresLectura)) {
		switch(recv(socketCliente, buffer, 1024, 0)) {
				case -1: _exit_with_error(socketCliente, "No se pudo recibir la sentencia");
						break;

				case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el cliente"ANSI_COLOR_RESET);
						close(socketCliente); 		//CIERRO EL SOCKET
						flag = 0; 							//FLAG 0 PARA SALIR DEL WHILE CUANDO SE DESCONECTA
		//				args->socketCliente.fd = -1;			//LO VUELVO A SETEAR EN -1 PARA QUE FUTUROS CLIENTES OCUPEN SU LUGAR EN EL ARRAY
						break;

				default: printf(ANSI_COLOR_BOLDGREEN"Se recibio el mensaje por parte del cliente y dice: %s\n"ANSI_COLOR_RESET, (char*) buffer);
						break;

			}
		}
	}

	free(buffer);
	pthread_exit(NULL);
}

void crearHiloESI(struct Cliente socketCliente){
	pthread_t threadESI;

//	struct arg_struct* args = malloc(sizeof(socketCliente)+sizeof(int));
//	args->socket=socket;
//	args->socketCliente.fd=socketCliente.fd;
//	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if( pthread_create(&threadESI, NULL, (void *) recibirSentenciaESI, (void *) socketCliente.fd )!= 0 ){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo instancia"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo ESI"ANSI_COLOR_RESET);

	pthread_detach(threadESI);
}

void aceptarCliente(int socket, struct Cliente* socketCliente) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
	fd_set descriptores;

	FD_ZERO(&descriptores);
	FD_SET(socket, &descriptores);

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Se esta esperando por nuevas conexiones..."ANSI_COLOR_RESET);

	for (int i=0; i<NUMEROCLIENTES; i++) {			//RECORRO EL ARRAY DE CLIENTES
		if (socketCliente[i].fd == -1) {			//SI ES IGUAL A -1, ES PORQUE TODAVIA NINGUN FILEDESCRIPTOR ESTA EN ESA POSICION

			select(5, &descriptores, NULL, NULL, NULL);

			if (FD_ISSET(socket, &descriptores)) {
			socketCliente[i].fd = accept(socket, (struct sockaddr *) &addr, &addrlen);		//ASIGNO FD AL ARRAY

			if (socketCliente[i].fd < 0) {
				_exit_with_error(socket, "No se pudo conectar el cliente");		// MANEJO DE ERRORES
			}

			if(envioHandshake(socketCliente[i].fd) < 0) {
				_exit_with_error(socket, "No se pudo enviar el handshake");
			}

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo enviar el handshake correctamente"ANSI_COLOR_RESET);

			switch(reciboIdentificacion(socketCliente[i].fd)) {
				case 0: _exit_with_error(socket, "No se pudo recibir el mensaje del protocolo de conexion"); //FALLO EL RECV
						break;
				case 1: asignarNombreAlSocketCliente(&socketCliente[i], "Planificador");
						crearHiloPlanificador(socketCliente[i]);
						break;
				case 2: asignarNombreAlSocketCliente(&socketCliente[i], "Instancia");
						crearHiloInstancia(socketCliente[i]);
						break;
				case 3: asignarNombreAlSocketCliente(&socketCliente[i], "ESI");
						crearHiloESI(socketCliente[i]);
						break;
				default: _exit_with_error(socket, ANSI_COLOR_RED"No estas cumpliendo con el protocolo de conexion"ANSI_COLOR_RESET);
						break;
			}

			log_info(logger, ANSI_COLOR_BOLDCYAN"Se pudo conectar el/la %s (FD %d) y esta en la posicion %d del array"ANSI_COLOR_RESET, socketCliente[i].nombre, socketCliente[i].fd, i);
			break;
		}
		}
	}

}

void recibirMensaje(void* argumentos) {
	struct arg_struct* args = argumentos;
	fd_set descriptoresLectura;
	int fdmax = args->socketCliente.fd + 1;
	int flag = 1;

	void* buffer = malloc(1024);
	int resultado_recv;

	while(flag) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(args->socketCliente.fd, &descriptoresLectura);

		select(fdmax, &descriptoresLectura, NULL, NULL, NULL);

		if (FD_ISSET(args->socketCliente.fd, &descriptoresLectura)) {
		switch(resultado_recv = recv(args->socketCliente.fd, buffer, 1024, 0)) {
				case -1: _exit_with_error(args->socket, "No se pudo recibir el mensaje del cliente");
						break;

				case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el cliente %s"ANSI_COLOR_RESET, args->socketCliente.nombre);
						close(args->socketCliente.fd); 		//CIERRO EL SOCKET
						free(args);							//LIBERO MEMORIA CUANDO SE DESCONECTA
						flag = 0; 							//FLAG 0 PARA SALIR DEL WHILE CUANDO SE DESCONECTA
		//				args->socketCliente.fd = -1;			//LO VUELVO A SETEAR EN -1 PARA QUE FUTUROS CLIENTES OCUPEN SU LUGAR EN EL ARRAY
						break;

				default: printf(ANSI_COLOR_BOLDGREEN"Se recibio el mensaje por parte del cliente %s de %d bytes y dice: %s\n"ANSI_COLOR_RESET, args->socketCliente.nombre, resultado_recv, (char*) buffer);
						break;

			}
		}
	}

	free(buffer);
	pthread_exit(NULL);
}


int envioHandshake(int socketCliente) {
	char* handshake = "******COORDINADOR HANDSHAKE******";

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Enviando handshake..."ANSI_COLOR_RESET);

	if(send(socketCliente, handshake, strlen(handshake)+1, 0) < 0) {
		return -1;
	}else{
		return 0;
	}
}


int reciboIdentificacion(int socketCliente) {
	char* identificador = malloc(sizeof(char));

	if(recv(socketCliente, identificador, sizeof(char)+1, MSG_WAITALL) < 0) {
		free(identificador);
		return 0;				//MANEJO EL ERROR EN ACEPTAR CLIENTE
	}

	if (strcmp(identificador, "1") == 0) {
		free(identificador);
		return 1;				//PLANIFICADOR
	} else if(strcmp(identificador, "2") == 0) {
		free(identificador);
		return 2;				//INSTANCIA
	} else if(strcmp(identificador, "3") == 0) {
		free(identificador);
		return 3;				//ESI
	}

	free(identificador);
	return -1;
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
	crearConfig();
	setearConfigEnVariables();

	int listenSocket = conectarSocketYReservarPuerto();

	escuchar(listenSocket);

	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i].fd = -1;				//INICIALIZO EL ARRAY DE FDS EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
	}

	while(1) {
		aceptarCliente(listenSocket, socketCliente);
	}

	close(listenSocket);

	return EXIT_SUCCESS;
}
