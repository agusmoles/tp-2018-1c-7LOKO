#include "coordinador.h"

cliente_t v_instanciasConectadas[NUMEROCLIENTES];
int instanciaSiguiente = 0;
int cantidadInstanciasConectadas = 0;
int identificadorInstancia = 0;
int idEsiEjecutando = 0;
int* tamanioValorStatus;
char* valorStatus;

/* Funciomes de conexion */
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

void configurarLogOperaciones(){
	logOperaciones = log_create("logOperaciones.log", "coordinador", 0, LOG_LEVEL_INFO);
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

int envioHandshake(int socketCliente) {
	char* handshake = "******COORDINADOR HANDSHAKE******";

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Enviando handshake..."ANSI_COLOR_RESET);

	if(send(socketCliente, handshake, strlen(handshake)+1, 0) < 0) {
		return -1;
	}else{
		return 0;
	}
}

void intHandler() {
	printf(ANSI_COLOR_BOLDRED"\n************************************SE INTERRUMPIO EL PROGRAMA************************************\n"ANSI_COLOR_RESET);
	exit(1);
}


/* Recibo clave, tamanio valor, valor, id esi */
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

void recibirIDdeESI(cliente_t* cliente){
	int *buffer = malloc(sizeof(int));

	if(recv(cliente->fd, buffer, sizeof(int), MSG_WAITALL)<0){
		_exit_with_error(cliente->fd, "No se pudo recibir el identificador ESI");
	}

	cliente->identificadorESI = *buffer;

	free(buffer);
	log_info(logger, ANSI_COLOR_BOLDCYAN "Se recibio el identificador del ESI %d"ANSI_COLOR_RESET, cliente->identificadorESI);
}

/* Recibe sentencia del ESI */
void recibirSentenciaESI(void* argumentos){
	header_t* buffer_header = malloc(sizeof(header_t));
	arg_esi_t* args = argumentos;
	int flag = 1;
	fd_set descriptoresLectura;

	while(flag) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(args->socketCliente->fd, &descriptoresLectura);

		select(args->socketCliente->fd + 1 , &descriptoresLectura, NULL, NULL, NULL);

		if (FD_ISSET(args->socketCliente->fd, &descriptoresLectura)) {
		switch(recv(args->socketCliente->fd, buffer_header, sizeof(header_t), MSG_WAITALL)){
				case -1: _exit_with_error(args->socketCliente->fd, ANSI_COLOR_BOLDRED"No se pudo recibir el header del ESI"ANSI_COLOR_RESET);
						break;

				case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el ESI"ANSI_COLOR_RESET);
						close(args->socketCliente->fd); 		//CIERRO EL SOCKET
						flag = 0; 						//FLAG 0 PARA SALIR DEL WHILE CUANDO SE DESCONECTA

						free(args);
						break;

				default: /* Si no hay errores */
						log_info(logger, ANSI_COLOR_BOLDWHITE"Header recibido. COD OP: %d - TAM: %d"ANSI_COLOR_RESET, buffer_header->codigoOperacion, buffer_header->tamanioClave);

						tratarSegunOperacion(buffer_header, args->socketCliente, args->socketPlanificador);

						//enviarMensaje(args->socketCliente->fd, "OPOK");
						break;
			}
		}
	}

	pthread_exit(NULL);
}

/* Asigna ID a cada Instancia para identificarlas */
void asignarIDdeInstancia(cliente_t* socketCliente, int id){
	socketCliente->identificadorInstancia = id;
	log_info(logger, ANSI_COLOR_BOLDCYAN "Se recibio el identificador de la Instancia %d"ANSI_COLOR_RESET, socketCliente->identificadorInstancia);
}

/* Creacion de hilos para cada cliente */
void crearHiloPlanificador(cliente_t socketCliente){
	pthread_t threadPlanificador;

	arg_t* args = malloc(sizeof(socketCliente)+sizeof(int));
	//args->socket=socket;
	args->socketCliente.fd = socketCliente.fd;
	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if(pthread_create(&threadPlanificador, NULL, (void *) recibirMensaje_Planificador, args)!=0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo planificador"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo planificador"ANSI_COLOR_RESET);
	pthread_detach(threadPlanificador);
}

void crearHiloInstancia(cliente_t socketCliente){
	pthread_t threadInstancia;

	arg_t* args = malloc(sizeof(socketCliente)+sizeof(int));
	//args->socket=socket;
	args->socketCliente.fd = socketCliente.fd;
	strcpy(args->socketCliente.nombre, socketCliente.nombre);

	if(pthread_create(&threadInstancia, NULL, (void *) recibirMensaje_Instancias, args)!=0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo instancia"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo instancia"ANSI_COLOR_RESET);
	pthread_detach(threadInstancia);
}

void crearHiloESI(cliente_t* socketCliente, int socketPlanificador){
	pthread_t threadESI;
	arg_esi_t* args = malloc(sizeof(cliente_t *) + sizeof(int));
	args->socketPlanificador = socketPlanificador;
	args->socketCliente = socketCliente;

	if( pthread_create(&threadESI, NULL, (void *) recibirSentenciaESI, (void*) args)!= 0 ){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo ESI"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo ESI"ANSI_COLOR_RESET);

	pthread_detach(threadESI);
}

void crearHiloStatus() {
	pthread_t threadStatus;

	if( pthread_create(&threadStatus, NULL, (void *) recibirMensajeStatus, NULL)!= 0 ){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo de status"ANSI_COLOR_RESET);
		exit(-1);
	}

	log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo de status"ANSI_COLOR_RESET);

	pthread_detach(threadStatus);
}


/* Funciones que llaman los hilos */
void recibirMensaje_Instancias(void* argumentos) {
	arg_t* args = argumentos;
	fd_set descriptoresLectura;
	int fdmax = args->socketCliente.fd + 1;
	int flag = 1;
	int resultado_recv;
	tamanioValorStatus =  malloc(sizeof(int));

	while(flag) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(args->socketCliente.fd, &descriptoresLectura);

		select(fdmax, &descriptoresLectura, NULL, NULL, NULL);

		if (FD_ISSET(args->socketCliente.fd, &descriptoresLectura)) {
		switch(resultado_recv = recv(args->socketCliente.fd, tamanioValorStatus, sizeof(int), 0)) {
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

				default:
						sem_wait(&semaforo_instancia);
						if(*tamanioValorStatus > 0){
							valorStatus = malloc(*tamanioValorStatus);
							if(recv(args->socketCliente.fd, valorStatus, *tamanioValorStatus, 0) < 0){
								_exit_with_error(args->socket, "No se pudo recibir el valor de la instancia");
							}
						}
						sem_post(&semaforo_instanciaOK);
						break;

			}
		}
	}
	pthread_exit(NULL);
}

void recibirMensaje_Planificador(void* argumentos) {
	arg_t* args = argumentos;
	fd_set descriptoresLectura;
	int fdmax = args->socketCliente.fd + 1;
	int flag = 1;
	int socketESI;

	while(flag) {
		FD_ZERO(&descriptoresLectura);
		FD_SET(args->socketCliente.fd, &descriptoresLectura);

		select(fdmax, &descriptoresLectura, NULL, NULL, NULL);

		if (FD_ISSET(args->socketCliente.fd, &descriptoresLectura)) {
			sem_wait(&semaforo_planificador);

			/* Buscar socket ESI */
			socketESI = buscarSocketESI(idEsiEjecutando);

			printf(ANSI_COLOR_BOLDWHITE"SOCKET ESI FD: %d\n"ANSI_COLOR_RESET, socketESI);

			if(verificarClaveTomada(args->socketCliente.fd) == 0){
				log_error(logger, ANSI_COLOR_BOLDRED"Error GET - STORE (Clave no tomada por el ESI) "ANSI_COLOR_RESET);
				enviarMensaje(socketESI, "OPBL");
			} else {
				log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo realizar el GET/STORE correctamente"ANSI_COLOR_RESET);
				enviarMensaje(socketESI, "OPOK");
			}

			sem_post(&semaforo_planificadorOK);
		}
	}
	pthread_exit(NULL);
}

void recibirMensajeStatus() {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
	fd_set descriptorLectura;
	int* tamanioClave = malloc(sizeof(int));
	char* clave;
	int instanciaEncargada;
	header_t* headerStatus = malloc(sizeof(header_t));
	int socketInstancia;

	if(listen(listenSocketStatus, 1) < 0) {
		_exit_with_error(listenSocketStatus, "No se puede esperar por conexiones del status");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se esta escuchando correctamente en el puerto del status"ANSI_COLOR_RESET);

	int socketStatusPlanificador = accept(listenSocketStatus, (struct sockaddr *) &addr, &addrlen);

	log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se conecto el comando status"ANSI_COLOR_RESET);

	while(1) {
		FD_ZERO(&descriptorLectura);
		FD_SET(socketStatusPlanificador, &descriptorLectura);

		select(socketStatusPlanificador + 1, &descriptorLectura, NULL, NULL, NULL);

		/* Primero recibo tamanio clave */
		switch (recv(socketStatusPlanificador, tamanioClave, sizeof(int), 0)) {

		case -1: _exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"No se pudo recibir el mensaje del comando status"ANSI_COLOR_RESET);
			break;

		case 0: close(listenSocketStatus);
			_exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"************SE DESCONECTO EL COMANDO STATUS************"ANSI_COLOR_RESET);
			break;

		default:
			log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se recibio el tamaño de la clave (status): %d"ANSI_COLOR_RESET, *tamanioClave);
			clave = malloc(*tamanioClave);

			/*Recibo clave*/
			if(recv(socketStatusPlanificador, clave, *tamanioClave, 0) < 0){
				_exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"No se pudo recibir la clave (status) "ANSI_COLOR_RESET);
			}
			log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se recibio la clave (status): %s"ANSI_COLOR_RESET, clave);


			/*Ahora envio instancia encargada */
			if((instanciaEncargada = buscarInstanciaEncargada(clave)) < 0){
				instanciaEncargada = seleccionEquitativeLoad();
			}

			/* Avisar a la instancia status */
			headerStatus->codigoOperacion = 3;
			headerStatus->tamanioClave = *tamanioClave;

			if((socketInstancia = buscarSocketInstancia(instanciaEncargada))< 0){
				_exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"No se encontro la instancia (status) "ANSI_COLOR_RESET);
			}

			enviarHeader(socketInstancia, headerStatus);
			enviarClave(socketInstancia, clave);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se enviaron header y clave a la instancia"ANSI_COLOR_RESET);

			/*Chequear si existe valor actual*/
			sem_post(&semaforo_instancia);

			sem_wait(&semaforo_instanciaOK);

			/*Envio el tamanio valor*/
			if(send(socketStatusPlanificador, tamanioValorStatus, sizeof(int), 0) < 0){
				_exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"No se pudo enviar el tamanio del valor (status) "ANSI_COLOR_RESET);
			}
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio el tamanio del valor %d al planificador"ANSI_COLOR_RESET, *tamanioValorStatus);

			if(*tamanioValorStatus > 0){
				/*Envio el valor*/
				if(send(socketStatusPlanificador, valorStatus, *tamanioValorStatus, 0) < 0){
					_exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"No se pudo enviar el valor (status) "ANSI_COLOR_RESET);
				}

				log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio el valor %s al planificador"ANSI_COLOR_RESET, valorStatus);
			}

			if(send(socketStatusPlanificador, &instanciaEncargada, sizeof(int), 0) < 0){
				_exit_with_error(socketStatusPlanificador, ANSI_COLOR_BOLDRED"No se pudo enviar la instancia (status) "ANSI_COLOR_RESET);
			}
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio la instancia encargada %d al planificador"ANSI_COLOR_RESET, instanciaEncargada);

			break;
		}
	}
}

int conectarSocketYReservarPuertoDeStatus() {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, "8002", &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE

	listenSocketStatus = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (listenSocketStatus <= 0) {
		_exit_with_error(listenSocketStatus, "No se pudo conectar el socket");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar el socket con éxito"ANSI_COLOR_RESET);

	if(bind(listenSocketStatus, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error(listenSocketStatus, "No se pudo reservar correctamente el puerto de escucha");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo reservar correctamente el puerto de escucha del servidor"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return listenSocketStatus;
}

/* Asigna nombre a cada cliente particular:  Instancia, ESI, Planificador */
void asignarNombreAlSocketCliente(cliente_t* socketCliente, char* nombre) {
	strcpy(socketCliente->nombre, nombre);
}

/* Crea un array con las instancias que se encuentran conectadas y muestra la cantidad*/
void instanciasConectadas(){
	printf("Cantidad de instancias conectadas: %d \n", cantidadInstanciasConectadas);
}

int buscarInstanciaEncargada(char* clave){
	for(int i=0; i<CANTIDADCLAVES; i ++){
		if(strcmp(clavesExistentes[i].clave, clave) == 0){
			return clavesExistentes[i].instancia;
		}
	}
	return -1;
}

void setearInstancia(char* clave, int instancia){
	for(int i=0; i<CANTIDADCLAVES; i ++){
		if(strcmp(clavesExistentes[i].clave, clave) == 0){
			clavesExistentes[i].instancia = instancia;
		}
	}
}

/* Si no hay instancias conectadas logea el error */
int verificarSiExistenInstanciasConectadas(){
	if(cantidadInstanciasConectadas == 0){
		log_error(logger,ANSI_COLOR_RED"No hay instancias conectadas: no se puede tratar al ESI"ANSI_COLOR_RESET);
		return -1;
	}
	return 1;
}

/* Busca y crea vector Instancias Conectadas */
void actualizarVectorInstanciasConectadas(){
	int h= 0;
	for(int i=0; i< NUMEROCLIENTES; i++){
		if(strcmp(socketCliente[i].nombre, "Instancia") == 0){
			v_instanciasConectadas[h] = socketCliente[i];
			h++;
		}
	}
}

/* Algoritmos de distribucion de las sentencias a las disitintas instancias */
int seleccionEquitativeLoad(){
	if (verificarSiExistenInstanciasConectadas() > 0){

		if(instanciaSiguiente < cantidadInstanciasConectadas){
			instanciaSiguiente++;
		return (instanciaSiguiente-1);
		}else {
			instanciaSiguiente = 1;
		return (instanciaSiguiente -1 );
		}

	} else {
		return -1;
	}
}

int seleccionLeastSpaceUsed(){
	int entradasOcupadasPorInstancia[cantidadInstanciasConectadas];

	for(int h=0; h<cantidadInstanciasConectadas; h++){
		entradasOcupadasPorInstancia[h] = 0;
	}

	/*Buscar entradas ocupadas de cada instancia*/
	for(int i=0; i<CANTIDADCLAVES; i++){
		switch(clavesExistentes[i].instancia){
		case 0:
			entradasOcupadasPorInstancia[0] ++;
			break;
		case 1:
			entradasOcupadasPorInstancia[1] ++;
			break;
		case 2:
			entradasOcupadasPorInstancia[2] ++;
			break;
		case 3:
			entradasOcupadasPorInstancia[3] ++;
			break;
		default:
			break;
		}
	}

	int minimo = entradasOcupadasPorInstancia[0];
	int instanciaSeleccionada = 0;

	/*Busco la que tenga menos entradas ocupadas*/
	for(int j=0; j<cantidadInstanciasConectadas; j++){
		if(entradasOcupadasPorInstancia[j] < minimo){
			minimo = entradasOcupadasPorInstancia[j];
			instanciaSeleccionada = j;
		}
	}

	return instanciaSeleccionada;
}

int seleccionKeyExplicit(char inicial){
	int cantidadLetras = 26;
	int i = 0;
	int numeroInstancias = cantidadInstanciasConectadas;
	int letrasUsadas;
	actualizarVectorInstanciasConectadas();

	/*Primero asigno letra a cada instancia */
	v_instanciasConectadas[0].primeraLetra = 'a';

	// Redondear para arriba -> 1 + ((x - 1) / y)
	while((cantidadLetras % numeroInstancias) != 0){
		letrasUsadas =  1 + ( (cantidadLetras - 1) / numeroInstancias);
		if(i > 0){
			v_instanciasConectadas[i].primeraLetra = v_instanciasConectadas[i-1].ultimaLetra + 1;
		}
		v_instanciasConectadas[i].ultimaLetra = v_instanciasConectadas[i].primeraLetra + letrasUsadas -1;
		i ++;
		cantidadLetras -= letrasUsadas;
		numeroInstancias --;
	}

	for(int j = i; j<cantidadInstanciasConectadas; j++){
		if(j > 0){
			v_instanciasConectadas[j].primeraLetra = v_instanciasConectadas[j-1].ultimaLetra + 1;
		}
		letrasUsadas = cantidadLetras / numeroInstancias;
		v_instanciasConectadas[j].ultimaLetra = v_instanciasConectadas[j].primeraLetra + letrasUsadas - 1;
	}

	/*Ahora selecciono la instancia encargada segun primer letra*/
	for(int h=0; h<cantidadInstanciasConectadas; h++){
		printf("Instancia %d: Primera letra %c - Ultima letra %c\n", h, v_instanciasConectadas[h].primeraLetra, v_instanciasConectadas[h].ultimaLetra);

		if(inicial >= v_instanciasConectadas[h].primeraLetra && inicial <= v_instanciasConectadas[h].ultimaLetra){
			return h;
		}
	}
	return -1;
}

/* Administracion de claves del coordinador */
int verificarSiExisteClave(char* clave){
	for(int i=0; i<CANTIDADCLAVES; i++){
		if(strcmp(clavesExistentes[i].clave, clave) == 0){
			return 1;
		}
	}
	return -1;
}

void agregarClave(int tamanioClave, char* clave){
	int flag = 0;

	for(int i=0; i<CANTIDADCLAVES; i++){
		if(strcmp(clavesExistentes[i].clave, "Nada") == 0 && flag == 0){
			clavesExistentes[i].clave = malloc(tamanioClave);
			strcpy(clavesExistentes[i].clave, clave);
			flag = 1;
		}
	}
	/* Si esta lleno el vector */
	if(flag == 0){
		log_error(logger, ANSI_COLOR_BOLDRED"No hay mas espacio para claves"ANSI_COLOR_RESET);
	}
}

void mostrarClavesExistentes(){
	for(int i=0; i<CANTIDADCLAVES; i++){
		printf("Clave: %s \n", clavesExistentes[i].clave);
	}
}

void enviarMensaje(int socketCliente, char* msg){
	if(send(socketCliente,msg, strlen(msg)+1,0)<0){
		_exit_with_error(socketCliente, "No se pudo enviar la sentencia");
	}
}

/* Envios header, clave, valor, id esi */
void enviarHeader(int socket, header_t* header) {
	if (send(socket, header, sizeof(header_t), 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el header"ANSI_COLOR_RESET);
	}
}

void enviarClave(int socket, char* clave) {
	if (send(socket, clave, strlen(clave)+1, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar la clave"ANSI_COLOR_RESET);
	}
}

void enviarValor(int socket, char* valor) {
	int32_t* tamanioValor = malloc(sizeof(int32_t));

	*tamanioValor = strlen(valor) +1;

	if (send(socket, tamanioValor, sizeof(int32_t), 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el tamanio del valor"ANSI_COLOR_RESET);
	}

	if (send(socket, valor, *tamanioValor, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el valor"ANSI_COLOR_RESET);
	}
	free(tamanioValor);
}

void enviarIDEsi(int socket, int idESI){
	int* idEsi = malloc(sizeof(int));

	*idEsi = idESI;

	if(send(socket, idEsi, sizeof(int), 0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el ID del ESI"ANSI_COLOR_RESET);
	}
	free(idEsi);
}


/*Envia la sentencia a Instancia - Planificador*/
void enviarSentenciaESI(int socket, header_t* header, char* clave, char* valor){
	enviarHeader(socket, header);
	enviarClave(socket, clave);
	enviarValor(socket, valor);
}

void enviarSentenciaESIStore(int socket, header_t* header, char* clave){
	enviarHeader(socket, header);
	enviarClave(socket, clave);
}

void enviarSentenciaAPlanificador(int socket, header_t* header, char* clave, int idESI){
	enviarHeader(socket, header);
	enviarClave(socket, clave);
	enviarIDEsi(socket, idESI);
}

int verificarClaveTomada(int socket){
	int* resultado = malloc(sizeof(int));
	if(recv(socket, resultado, sizeof(int), 0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se recibio el resultado de la operacion por el planificador"ANSI_COLOR_RESET);
		return -1;
	}
	return *resultado;
}

/* Realiza GET, SET, STORE */
void tratarSegunOperacion(header_t* header, cliente_t* socketESI, int socketPlanificador){
	char* bufferClave = malloc(header->tamanioClave);
	int instanciaEncargada;
	int32_t * tamanioValor;
	char* bufferValor;

	sem_wait(&mutexEsiEjecutando);
	idEsiEjecutando = socketESI->identificadorESI;
	sem_post(&mutexEsiEjecutando);

	switch(header->codigoOperacion){
		case 0: /* GET */
			recibirClave(socketESI->fd, header,bufferClave);

			/* Agregar clave nueva */
			if(verificarSiExisteClave(bufferClave) < 0){
				agregarClave(header->tamanioClave, bufferClave);
			}
			//mostrarClavesExistentes();

			/* Avisa a Planificador */
			enviarSentenciaAPlanificador(socketPlanificador, header, bufferClave, socketESI->identificadorESI);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se enviaron correctamente al Planificador: header - clave - idESI"ANSI_COLOR_RESET);

			sem_post(&semaforo_planificador);

			sem_wait(&semaforo_planificadorOK);
			/*Logea sentencia */
			log_info(logOperaciones, "ESI %d: OPERACION: GET %s", socketESI->identificadorESI, bufferClave);
			break;
		case 1: /* SET */
			/* Primero recibo all*/
			tamanioValor = malloc(sizeof(int32_t));
			recibirClave(socketESI->fd, header, bufferClave);

			/* Chequear que exista la clave */
			if(verificarSiExisteClave(bufferClave) < 0){
				log_error(logger, ANSI_COLOR_BOLDRED"Error de Clave no Identificada"ANSI_COLOR_RESET);
			}

			/* Avisa a Planificador */
			enviarSentenciaAPlanificador(socketPlanificador, header, bufferClave, socketESI->identificadorESI);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se enviaron correctamente al Planificador: header - clave - idESI"ANSI_COLOR_RESET);

			recibirTamanioValor(socketESI->fd, tamanioValor);
			bufferValor = malloc(*tamanioValor);
			recibirValor(socketESI->fd, tamanioValor, bufferValor);

			/*Ahora envio la sentencia a la Instancia encargada */
			if((instanciaEncargada = buscarInstanciaEncargada(bufferClave)) == -1){
				//instanciaEncargada = seleccionEquitativeLoad();
				//instanciaEncargada = seleccionLeastSpaceUsed();
				instanciaEncargada = seleccionKeyExplicit(bufferClave[0]);
				setearInstancia(bufferClave, instanciaEncargada);
			}
			printf(ANSI_COLOR_BOLDCYAN"-> La sentencia sera tratada por la Instancia %d \n"ANSI_COLOR_RESET, instanciaEncargada);
			actualizarVectorInstanciasConectadas();

			/* Faltaria verificar que se pueda realizar el send -> que este conectada la instancia */
			enviarSentenciaESI(v_instanciasConectadas[instanciaEncargada].fd, header, bufferClave, bufferValor);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se enviaron correctamente a la instancia: header - clave - tamanio_valor - valor"ANSI_COLOR_RESET);

			/*Falta respuesta de la instancia */

			enviarMensaje(socketESI->fd, "OPOK");
			/*Logea sentencia */
			log_info(logOperaciones, "ESI %d: OPERACION: SET %s %s",socketESI->identificadorESI, bufferClave, bufferValor);

			free(tamanioValor);
			free(bufferValor);
			break;
		case 2: /* STORE */
			recibirClave(socketESI->fd, header,bufferClave);

			/* Chequear que exista la clave */
			if(verificarSiExisteClave(bufferClave) < 0){
				log_error(logger, ANSI_COLOR_BOLDRED"Error de Clave no Identificada"ANSI_COLOR_RESET);
			}

			/* Avisa a Planificador */
			enviarSentenciaAPlanificador(socketPlanificador, header, bufferClave, socketESI->identificadorESI);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se enviaron correctamente al Planificador: header - clave - idESI"ANSI_COLOR_RESET);

			sem_post(&semaforo_planificador);

			sem_wait(&semaforo_planificadorOK);

			/*Ahora envio la sentencia a la Instancia encargada */
			if((instanciaEncargada = buscarInstanciaEncargada(bufferClave)) == -1){
				instanciaEncargada = seleccionEquitativeLoad();
				//instanciaEncargada = seleccionLeastSpaceUsed();
			}
			printf(ANSI_COLOR_BOLDCYAN"-> La sentencia sera tratada por la Instancia %d \n"ANSI_COLOR_RESET, instanciaEncargada);
			actualizarVectorInstanciasConectadas();

			/* Faltaria verificar que se pueda realizar el send -> que este conectada la instancia */
			enviarSentenciaESIStore(v_instanciasConectadas[instanciaEncargada].fd, header, bufferClave);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se enviaron correctamente a la instancia: header - clave "ANSI_COLOR_RESET);

			/*Falta respuesta de la instancia */

			/*Logea sentencia */
			log_info(logOperaciones, "ESI %d: OPERACION: STORE %s", socketESI->identificadorESI, bufferClave);
			break;
		default:
			_exit_with_error(socketESI->fd, "No cumpliste el protocolo de enviar Header");
	}
	free(bufferClave);
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

/* Maneja todos los clientes que se pueden conectar */
void aceptarCliente(int socket, cliente_t* socketCliente) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);
	fd_set descriptores;
	int socketPlanificador;

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

						/*Buscar el socket del planificador */
						if((socketPlanificador = buscarSocketPlanificador())< 0){
							_exit_with_error(socketCliente[i].fd, "No se encontro el socket del Planificador");
						}

						crearHiloESI(&socketCliente[i], socketPlanificador);
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

int buscarSocketPlanificador(){
	for(int i = 0; i<NUMEROCLIENTES; i++){
		if(strcmp(socketCliente[i].nombre, "Planificador") == 0){
			return socketCliente[i].fd;
		}
	}
	return -1;
}

int buscarSocketESI(int idEsi){
	for(int i = 0; i<NUMEROCLIENTES; i++){
		if(socketCliente[i].identificadorESI == idEsi){
			return socketCliente[i].fd;
		}
	}
	return -1;
}

int buscarSocketInstancia(int idInstancia){
	for(int i = 0; i<NUMEROCLIENTES; i++){
		if(socketCliente[i].identificadorInstancia == idInstancia){
			return socketCliente[i].fd;
		}
	}
	return -1;
}

cliente_t* buscarESI(int* IDESI) {
	for(int i=0; i<NUMEROCLIENTES; i++) {
		if(socketCliente[i].identificadorESI == *IDESI) {
			return &socketCliente[i];
		}
	}
	return NULL;
}

int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = intHandler;
	sigaction(SIGINT, &finalizacion, NULL);

	configurarLogger();
	crearConfig();
	setearConfigEnVariables();

	int listenSocket = conectarSocketYReservarPuerto();
	listenSocketStatus = conectarSocketYReservarPuertoDeStatus();

	escuchar(listenSocket);

	configurarLogOperaciones();

	crearHiloStatus();

	//INICIALIZO EL ARRAY DE FDS EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
	//INICIALIZO EL ARRAY DE INSTANCIAS CONECTADAS EN -1 LAS ID
	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i].fd = -1;
		v_instanciasConectadas[i].identificadorInstancia = -1;
		socketCliente[i].identificadorESI = -1;
		socketCliente[i].identificadorInstancia = -1;
	}

	for(int j=0; j<CANTIDADCLAVES; j++){
		char* flag = "Nada";
		int tamanioFlag = strlen(flag) + 1;
		clavesExistentes[j].clave = malloc(tamanioFlag);
		strcpy(clavesExistentes[j].clave, "Nada");
		clavesExistentes[j].instancia = -1;
	}

	sem_init(&semaforo_planificador, 0 , 0);
	sem_init(&semaforo_planificadorOK, 0 , 0);
	sem_init(&mutexEsiEjecutando, 0, 1);
	sem_init(&semaforo_instancia, 0, 0);
	sem_init(&semaforo_instanciaOK, 0,0);

	while(1) {
		aceptarCliente(listenSocket, socketCliente);
	}

	close(listenSocket);

	return EXIT_SUCCESS;
}
