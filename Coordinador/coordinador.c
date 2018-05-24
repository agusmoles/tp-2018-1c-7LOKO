/******** SERVIDOR COORDINADOR *********/
#include "coordinador.h"

cliente v_instanciasConectadas[NUMEROCLIENTES];

int instanciaSiguiente = 0;
int cantidadInstanciasConectadas = 0;
int identificadorInstancia = 0;

/* FUNCIONES DE SOCKET */

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

/* Asigna nombre a cada cliente particular:  Instancia, ESI, Planificador */
void asignarNombreAlSocketCliente(struct Cliente* socketCliente, char* nombre) {
	strcpy(socketCliente->nombre, nombre);
}

void enviarMensaje(int socketCliente, char* msg){
	if(send(socketCliente,msg, strlen(msg)+1,0)<0){
		_exit_with_error(socketCliente, "No se pudo recibir la sentencia");
	}
}


void recibirSegunOperacion(header* header, int socket){
	paquete* paquete = NULL;
	int tamanio = header->tamanoClave + header->tamanoValor + 1;

	char* buffer = malloc(tamanio);

	if(recv(socket, buffer, tamanio , 0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir la clave/valor de la Sentencia"ANSI_COLOR_RESET);
	}

	desempaquetar(buffer, header, paquete);

	printf(ANSI_COLOR_BOLDGREEN"Se recibio la sentencia del ESI y dice: %d %s %s\n"ANSI_COLOR_RESET, header->codigoOperacion, paquete->clave, paquete->valor);

	switch(header->codigoOperacion){
	case 0: /* GET */
		/* Avisar al planificador */

		break;
	case 1: /* SET */
		/*Avisa a Instancia encargada */


		break;
	case 2: /* STORE */
		/* Avisar al planificador */

		break;
	}
	free(buffer);
	free(paquete);
	free(header);
}

/* Recibe sentencia del ESI */
void recibirSentenciaESI(void* argumento){
	header* buffer_header = malloc(sizeof(header));

	cliente* socketCliente = (cliente*) argumento;
	int flag = 1;
	int instanciaEncargada;
	fd_set descriptoresLectura;

	while(flag) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(socketCliente->fd, &descriptoresLectura);

		select(socketCliente->fd + 1 , &descriptoresLectura, NULL, NULL, NULL);

		if (FD_ISSET(socketCliente->fd, &descriptoresLectura)) {
		switch(recv(socketCliente->fd, buffer_header, sizeof(header), 0)){
				case -1: _exit_with_error(socketCliente->fd, ANSI_COLOR_BOLDRED"No se pudo recibir el header de la Sentencia"ANSI_COLOR_RESET);
						break;

				case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el ESI"ANSI_COLOR_RESET);
						close(socketCliente->fd); 		//CIERRO EL SOCKET
						flag = 0; 						//FLAG 0 PARA SALIR DEL WHILE CUANDO SE DESCONECTA
						break;

				default:
					log_info(logger, "Header recibido %d \n", buffer_header->codigoOperacion);
					recibirSegunOperacion(buffer_header, socketCliente->fd);
//							instanciaEncargada = seleccionEquitativeLoad();
//							printf("La sentencia sera tratada por la Instancia %d \n", instanciaEncargada);
//							enviarSentenciaESIaInstancia(v_instanciasConectadas[instanciaEncargada].fd, buffer);
//							printf(ANSI_COLOR_BOLDGREEN"Se envio la sentencia %s a la Instancia %d \n"ANSI_COLOR_RESET, (char*) buffer, instanciaEncargada);

							enviarMensaje(socketCliente->fd, "OPOK");
							break;
			}
		}
	}

	pthread_exit(NULL);
}

void recibirIDdeESI(cliente* cliente){
	int *buffer = malloc(sizeof(int));

	if(recv(cliente->fd, buffer, sizeof(int), MSG_WAITALL)<0){
		_exit_with_error(cliente->fd, "No se pudo recibir el identificador ESI");
	}

	cliente->identificadorESI = *buffer;

	free(buffer);
	log_info(logger, ANSI_COLOR_BOLDCYAN "Se recibio el identificador del ESI %d"ANSI_COLOR_RESET, cliente->identificadorESI);
}

/* Asigna ID a cada Instancia para identificarlas */
void asignarIDdeInstancia(struct Cliente* socketCliente, int id){
	socketCliente->identificadorInstancia = id;
	log_info(logger, ANSI_COLOR_BOLDCYAN "Se recibio el identificador de la Instancia %d"ANSI_COLOR_RESET, socketCliente->identificadorInstancia);
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
						if(strcmp(args->socketCliente.nombre, "Instancia") == 0){
							cantidadInstanciasConectadas--;
							instanciasConectadas();
						}
						close(args->socketCliente.fd); 		//CIERRO EL SOCKET
						free(args);							//LIBERO MEMORIA CUANDO SE DESCONECTA
						flag = 0; 							//FLAG 0 PARA SALIR DEL WHILE CUANDO SE DESCONECTA
						break;

				default: printf(ANSI_COLOR_BOLDGREEN"Se recibio el mensaje por parte del cliente %s de %d bytes y dice: %s\n"ANSI_COLOR_RESET, args->socketCliente.nombre, resultado_recv, (char*) buffer);
						break;

			}
		}
	}

	free(buffer);
	pthread_exit(NULL);
}

/* CREACION DE HILOS PARA CADA CLIENTE */

void crearHiloPlanificador(cliente socketCliente){
	pthread_t threadPlanificador;

	struct arg_struct* args = malloc(sizeof(socketCliente)+sizeof(int));
	//args->socket=socket;
	args->socketCliente.fd = socketCliente.fd;
	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if(pthread_create(&threadPlanificador, NULL, (void *) recibirMensaje, args)!=0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo planificador"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo planificador"ANSI_COLOR_RESET);
	pthread_detach(threadPlanificador);
}

void crearHiloInstancia(cliente socketCliente){
	pthread_t threadInstancia;

	struct arg_struct* args = malloc(sizeof(socketCliente)+sizeof(int));
	//args->socket=socket;
	args->socketCliente.fd = socketCliente.fd;
	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if(pthread_create(&threadInstancia, NULL, (void *) recibirMensaje, args)!=0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo instancia"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo instancia"ANSI_COLOR_RESET);
	pthread_detach(threadInstancia);
}

void crearHiloESI(cliente socketCliente){
	pthread_t threadESI;
//	struct arg_struct* args = malloc(sizeof(socketCliente)+sizeof(int));
//	args->socket=socket;
//	args->socketCliente.fd=socketCliente.fd;
//	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if( pthread_create(&threadESI, NULL, (void *) recibirSentenciaESI, (void*) &socketCliente)!= 0 ){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo ESI"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo ESI"ANSI_COLOR_RESET);

	pthread_detach(threadESI);
}

/* Crea un array con las instancias que se encuentran conectadas y muestra la cantidad*/
void instanciasConectadas(){
	printf("Cantidad de instancias conectadas: %d \n", cantidadInstanciasConectadas);
}

/* Si no hay instancias conectadas logea el error */
int verificarSiExistenInstanciasConectadas(){
	if(cantidadInstanciasConectadas == 0){
		log_error(logger,ANSI_COLOR_RED"No hay instancias conectadas: no se puede tratar al ESI"ANSI_COLOR_RESET);
		return -1;
	}
	return 1;
}

/* Distribuye las sentencias a las disitintas instancias */
int seleccionEquitativeLoad(){
	if (verificarSiExistenInstanciasConectadas() > 0){

		if(instanciaSiguiente < cantidadInstanciasConectadas){
			instanciaSiguiente++;
		return (instanciaSiguiente-1);
		}else {
			instanciaSiguiente = 0;
		return instanciaSiguiente;
		}

	} else {
		return -1;
	}
}

/*Envia la sentencia a la instancia correspondiente*/
void enviarSentenciaESIaInstancia(int socket, char* sentencia){
	int h=0;

	for(int i=0; i< NUMEROCLIENTES; i++){
		if(strcmp(socketCliente[i].nombre, "Instancia") == 0){
			v_instanciasConectadas[h] = socketCliente[i];
			h++;
		}
	}

	if(send(socket, sentencia, sizeof(sentencia),0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar la sentencia a la Instancia"ANSI_COLOR_RESET);
	}

	//
//	for(int j=0; j<cantidadInstanciasConectadas; j++){
//		if(v_instanciasConectadas[j].identificadorInstancia == id_instancia){
//			if( send(socket, sentencia, sizeof(sentencia),0) < 0){
//				_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar la sentencia a la Instancia"ANSI_COLOR_RESET);
//			}
//		}else{
//			log_error(logger, "La Instancia %d no esta conectada\n", id_instancia);
//		}
//	}
}

/* Maneja todos los clientes que se pueden conectar */
void aceptarCliente(int socket, cliente* socketCliente) {
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
						asignarIDdeInstancia(&socketCliente[i], identificadorInstancia);
						crearHiloInstancia(socketCliente[i]);
						identificadorInstancia++;
						cantidadInstanciasConectadas++;
						break;

				case 3: asignarNombreAlSocketCliente(&socketCliente[i], "ESI");
						recibirIDdeESI(&socketCliente[i]);
						crearHiloESI(socketCliente[i]);
						break;

				default: _exit_with_error(socket, ANSI_COLOR_RED"No estas cumpliendo con el protocolo de conexion"ANSI_COLOR_RESET);
						break;
			}

			instanciasConectadas();
			log_info(logger, ANSI_COLOR_BOLDCYAN"Se pudo conectar el/la %s (FD %d) y esta en la posicion %d del array"ANSI_COLOR_RESET, socketCliente[i].nombre, socketCliente[i].fd, i);

			break;
		}
		}
	}

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

/* Identifica si se conecto ESI, PLANIFICADOR o INSTANCIA */
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

	//INICIALIZO EL ARRAY DE FDS EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i].fd = -1;
	}

	while(1) {
		aceptarCliente(listenSocket, socketCliente);
	}



	close(listenSocket);

	return EXIT_SUCCESS;
}
