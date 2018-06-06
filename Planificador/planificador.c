/******** SERVIDOR PLANIFICADOR *********/

#include "planificador.h"
#include "consola.h"


void _exit_with_error(char* mensaje) {
	close(listenSocket);

	for(int i=0; i<NUMEROCLIENTES; i++) {
		if (socketCliente[i].fd != -1) {
			close(socketCliente[i].fd);					// CIERRO TODOS LOS SOCKETS DE CLIENTES QUE ESTABAN ABIERTOS EN EL ARRAY
		}
	}

	FD_ZERO(&descriptoresLectura);

	log_error(logger, mensaje);
	exit(1);
}

void configurarLogger() {
	logger = log_create("planificador.log", "planificador", 1, LOG_LEVEL_INFO);
}

void crearConfig() {
	config = config_create("../cfg");
}

void crearDiccionarioDeClaves() {
	diccionarioClaves = dictionary_create();
}

void setearConfigEnVariables() {
	PUERTOPLANIFICADOR = config_get_string_value(config, "Puerto Planificador");
	PUERTOCOORDINADOR = config_get_string_value(config, "Puerto Coordinador");
	IPCOORDINADOR = config_get_string_value(config, "IP Coordinador");
	algoritmoPlanificacion = config_get_string_value(config, "Algoritmo de planificación");
	alfaPlanificacion = config_get_double_value(config, "Alfa planificación");
	estimacionInicial = config_get_int_value(config, "Estimación inicial");
	sem_init(&pausado, 0, 1);	//SETEO EL SEMAFORO PAUSADO EN 1 PORQUE INICIALMENTE NO ESTA BLOQUEADO
	sem_init(&mutexListos, 0, 1);
	sem_init(&mutexEjecutando, 0, 1);
	sem_init(&mutexBloqueados, 0, 1);
	sem_init(&mutexFinalizados, 0, 1);

	/************************ SETEO CLAVES BLOQUEADAS ******************/

	char* claves = config_get_string_value(config, "Claves inicialmente bloqueadas");

	if (claves != NULL) {
		char** clavesSeparadas = string_split(claves, ",");

		for (int i=0; clavesSeparadas[i] != NULL; i++) {		// SIGO HASTA QUE SEA NULL EL ARRAY
			dictionary_put(diccionarioClaves, clavesSeparadas[i], "SISTEMA");	// AGREGO LA CLAVE Y QUIEN LA TOMA ES EL "SISTEMA"
			free(clavesSeparadas[i]);							// LIBERO LA MEMORIA TOMADA DE LA CLAVE
		}

		free(clavesSeparadas);
	}
}

void setearListaDeEstados() {
	listos = list_create();
	finalizados = list_create();
	ejecutando = list_create();
	bloqueados = list_create();
}

int conectarSocketYReservarPuerto() {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(NULL, PUERTOPLANIFICADOR, &hints, &serverInfo); // Notar que le pasamos NULL como IP, ya que le indicamos que use localhost en AI_PASSIVE

	int listenSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (listenSocket <= 0) {
		_exit_with_error("No se pudo conectar el socket");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar el socket con éxito"ANSI_COLOR_RESET);

	if(bind(listenSocket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error("No se pudo reservar correctamente el puerto de escucha");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo reservar correctamente el puerto de escucha del servidor"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return listenSocket;
}

/******************************************************** CONEXION COORDINADOR INICIO *****************************************************/

int conectarSocketCoordinador() {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(IPCOORDINADOR, PUERTOCOORDINADOR, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

	int server_socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (connect(server_socket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error("No se pudo conectar con el servidor");
	}

	log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se pudo conectar con el Coordinador"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return server_socket;
}

void reciboHandshake(int socket) {
	char* handshake = "******COORDINADOR HANDSHAKE******";
	char* buffer = malloc(strlen(handshake)+1);


	switch (recv(socket, buffer, strlen(handshake)+1, MSG_WAITALL)) {
		case -1: _exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el handshake"ANSI_COLOR_RESET);
				break;
		case 0:  _exit_with_error(ANSI_COLOR_BOLDRED"Se desconecto el servidor forzosamente"ANSI_COLOR_RESET);
				break;
		default: if (strcmp(handshake, buffer) == 0) {
					log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se recibio el handshake correctamente"ANSI_COLOR_RESET);
				}
				break;
	}

	free(buffer);
}

void envioIdentificador(int socket) {
	char* identificador = "1";			//PLANIFICADOR ES 1

	if (send(socket, identificador, strlen(identificador)+1, 0) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo enviar el identificador"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDWHITE"Se envio correctamente el identificador"ANSI_COLOR_RESET);
}

void conectarConCoordinador() {
	int socket = conectarSocketCoordinador();

	reciboHandshake(socket);
	envioIdentificador(socket);

	fd_set descriptorCoordinador;
	int fdmax;
	header_t* buffer_header = malloc(sizeof(header_t));
	int* IDESI = malloc(sizeof(int));
	char* clave;

	while(1) {
		FD_ZERO(&descriptorCoordinador);
		FD_SET(socket, &descriptorCoordinador);
		fdmax = socket;

		select(fdmax + 1, &descriptorCoordinador, NULL, NULL, NULL);

		if (FD_ISSET(socket, &descriptorCoordinador)) {

			recibirHeader(socket, buffer_header);

			clave = malloc(buffer_header->tamanioClave);

			recibirClave(socket, buffer_header->tamanioClave, clave);

			recibirIDDeESI(socket, IDESI);

			char* nombreESI = malloc(7);	// 7 PORQUE ES "ESI ID", DEJO ESPACIO PARA ESIS DE MAS DE DOS CIFRAS

			strcpy(nombreESI, "ESI ");

			char* idEsi = string_itoa(*IDESI);  // PASO EL ID A STRING ASI LO CONCATENO

			strcat(nombreESI, idEsi);	// CONCATENO "ESI" CON EL ID...

			free(idEsi);	// LIBERO EL STRING QUE SOLO TIENE EL ID...

			switch(buffer_header->codigoOperacion) {
			case 0: // OPERACION GET
				if (dictionary_has_key(diccionarioClaves, clave)) {

					bloquearESI(clave, IDESI);

					informarAlCoordinador(socket, 0);	// 0 PORQUE SALIO MAL

					char* IDEsiQueTieneLaClaveTomada = dictionary_get(diccionarioClaves, clave);

					log_error(logger, ANSI_COLOR_BOLDRED"La clave %s ya estaba tomada por el %s. Se bloqueo al %s"ANSI_COLOR_RESET, clave, IDEsiQueTieneLaClaveTomada, nombreESI);

				} else {

					dictionary_put(diccionarioClaves, clave, nombreESI);

					log_info(logger, ANSI_COLOR_BOLDCYAN"El %s tomo efectivamente la clave %s"ANSI_COLOR_RESET, nombreESI, clave);

					informarAlCoordinador(socket, 1); // 1 PORQUE SALIO BIEN
				}
				break;
			case 2: //OPERACION STORE
				if (dictionary_has_key(diccionarioClaves, clave)) {
					char* IDEsiQueTieneLaClaveTomada = dictionary_get(diccionarioClaves, clave);

					if(strcmp(nombreESI, IDEsiQueTieneLaClaveTomada) == 0) {	// SI EL QUE QUIERE STOREAR ES EL MISMO QUE LA TIENE STOREADA, OK.
						dictionary_remove(diccionarioClaves, clave);
						log_info(logger, ANSI_COLOR_BOLDCYAN"Se removio la clave %s tomada por el %s"ANSI_COLOR_RESET, clave, nombreESI);

						informarAlCoordinador(socket, 1); // 1 PORQUE SALIO BIEN

						// DESBLOQUEO AL ESI QUE ESTA ESPERANDO POR ESTE RECURSO, SI LO HUBIESE

						desbloquearESI(clave);

					} else {		// DEBO ABORTAR AL ESI POR STOREAR UNA CLAVE QUE NO ES DE EL
						abortarESI(IDESI, nombreESI);

						log_error(logger, ANSI_COLOR_BOLDRED"Se aborto el %s por querer hacer STORE de la clave %s que no es de el"ANSI_COLOR_RESET, nombreESI, clave);

						informarAlCoordinador(socket, 0); // 0 PORQUE SALIO MAL

						ordenarProximoAEjecutar();	// ORDENO PROXIMO A EJECUTAR YA QUE ABORTE A UN ESI...
					}
				} else {		// SI LA CLAVE NO ESTA EN EL DICCIONARIO...
					abortarESI(IDESI, nombreESI);

					log_error(logger, ANSI_COLOR_BOLDRED"Se aborto el %s por querer hacer STORE de la clave %s inexistente"ANSI_COLOR_RESET, nombreESI, clave);

					informarAlCoordinador(socket, 0); // 0 PORQUE SALIO MAL

					ordenarProximoAEjecutar();	// ORDENO PROXIMO A EJECUTAR YA QUE ABORTE A UN ESI...
				}

				break;
			default:
				_exit_with_error(ANSI_COLOR_BOLDRED"No se esperaba un codigo de operacion distinto de 0 (GET) o 2 (STORE) del Coordinador"ANSI_COLOR_RESET);
				break;
			}

			free(clave);
		}
	}

	free(IDESI);
}

void eliminarClavesTomadasPorEsiFinalizado(char* clave, void* ESI) {
	if(strcmp(ESI, ESIABuscarEnDiccionario) == 0) {
		log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se elimino la clave %s tomada por el %s ya que fue abortado"ANSI_COLOR_RESET, clave, (char*) ESI);
		dictionary_remove(diccionarioClaves, clave);
	}
}

cliente* buscarESI(int* IDESI) {
	for(int i=0; i<NUMEROCLIENTES; i++) {
		if(socketCliente[i].identificadorESI == *IDESI) {
			return &socketCliente[i];
		}
	}
	return NULL;
}

void abortarESI(int* IDESI, char* nombreESI) {
	ESIABuscarEnDiccionario = malloc(strlen(nombreESI)+1);

	strcpy(ESIABuscarEnDiccionario, nombreESI);		// SETEO LA VARIABLE GLOBAL

	dictionary_iterator(diccionarioClaves, (void*) eliminarClavesTomadasPorEsiFinalizado); // REMUEVO LAS CLAVES TOMADAS POR EL ESI A FINALIZAR

	free(ESIABuscarEnDiccionario);

	sem_wait(&mutexEjecutando);
	list_remove(ejecutando, 0);
	sem_post(&mutexEjecutando);

	cliente* ESIAbortado = buscarESI(IDESI);

	close(ESIAbortado->fd);
	ESIAbortado->fd = -1;	// PARA QUE NO LO AGREGUE AL SET DE DESCRIPTORES DE LECTURA DEL MANEJODECLIENTES()

	sem_wait(&mutexFinalizados);
	list_add(finalizados, ESIAbortado);
	sem_post(&mutexFinalizados);
}

void bloquearESI(char* clave, int* IDESI) {
	cliente* ESI = buscarESI(IDESI);
	strcpy(ESI->recursoSolicitado, clave);

	sem_wait(&mutexEjecutando);
	list_remove(ejecutando, 0);	// LO SACO DE EJECUTANDO
	sem_post(&mutexEjecutando);

	sem_wait(&mutexBloqueados);
	list_add(bloqueados, ESI);	// Y LO MUEVO A BLOQUEADOS
	sem_post(&mutexBloqueados);

	if(strncmp(algoritmoPlanificacion, "SJF", 3) == 0) {	// SI ES SJF, SETEO VARIABLES DEL ESI EJECUTANDO
		ESI->estimacionProximaRafaga = (alfaPlanificacion / 100) * ESI->rafagaActual + (1 - (alfaPlanificacion / 100) ) * ESI->estimacionRafagaActual;
		ESI->estimacionRafagaActual = ESI->estimacionProximaRafaga;
		ESI->rafagaActual = 0; 	// LO SETEO EN 0 PORQUE FUE "DESALOJADO" PORQUE SE BLOQUEO
	}

	if (strcmp(algoritmoPlanificacion, "HRRN") == 0) {
		ESI->estimacionProximaRafaga = (alfaPlanificacion / 100) * ESI->rafagaActual + (1 - (alfaPlanificacion / 100) ) * ESI->estimacionRafagaActual;
		ESI->estimacionRafagaActual = ESI->estimacionProximaRafaga;
		ESI->tiempoDeEspera = 0;

		sem_wait(&mutexListos);
		list_iterate(listos, (void *) sumarUnoAlWaitingTime);
		sem_post(&mutexListos);
	}

	if (list_size(listos) >= 2) {			//SI EN LISTOS HAY MAS DE 2 ESIS ORDENO...
		if(strncmp(algoritmoPlanificacion, "SJF", 3) == 0) { // SI ES SJF
			ordenarColaDeListosPorSJF();
		}

		else if (strcmp(algoritmoPlanificacion, "HRRN") == 0) { //SI ES HRRN
			sem_wait(&mutexListos);
			list_iterate(listos, (void*) calcularResponseRatio);	// CALCULO LA TASA DE RESPUESTA DE TODOS LOS ESIS LISTOS
			sem_post(&mutexListos);
			ordenarColaDeListosPorHRRN();
		}
	}

	ordenarProximoAEjecutar();	// COMO LO BLOQUEE, TENGO QUE MANDAR A OTRO A EJECUTAR
}

void desbloquearESI(char* clave) {
	cliente* primerESIBloqueadoEsperandoPorClave;

	for(int i=0; i<list_size(bloqueados); i++) {
		primerESIBloqueadoEsperandoPorClave = list_get(bloqueados, i);

		if (strcmp(primerESIBloqueadoEsperandoPorClave->recursoSolicitado, clave) == 0) {
			strcpy(primerESIBloqueadoEsperandoPorClave->recursoSolicitado, "");		// NO TIENE RECURSO PEDIDO AHORA...

			sem_wait(&mutexBloqueados);
			list_remove(bloqueados, i);		// LO SACO DE BLOQUEADOS AL ESI
			sem_post(&mutexBloqueados);

			sem_wait(&mutexListos);
			list_add(listos, primerESIBloqueadoEsperandoPorClave);	// LO MANDO A LISTOS
			sem_post(&mutexListos);

			log_info(logger, ANSI_COLOR_BOLDCYAN"Se desbloqueo al ESI %d ya que se libero la clave %s"ANSI_COLOR_RESET, primerESIBloqueadoEsperandoPorClave->identificadorESI, clave);

			break;	// SALGO DEL FOR PORQUE YA ENCONTRE AL PRIMERO
		}
	}

	if(list_is_empty(ejecutando)) {
		ordenarProximoAEjecutar();
	}
}

void listar(char* clave) {
	cliente* ESI;

	printf(ANSI_COLOR_BOLDWHITE"-----ESIS ESPERANDO POR CLAVE %s-------\n\n"ANSI_COLOR_RESET, clave);

	if (!list_is_empty(bloqueados)) {
		for(int i=0; i<list_size(bloqueados); i++) {
			ESI = list_get(bloqueados, i);

			if (strcmp(ESI->recursoSolicitado, clave) == 0) {
				printf(ANSI_COLOR_BOLDWHITE"ESI %d\n"ANSI_COLOR_RESET, ESI->identificadorESI);
			}
		}
	}
}

void informarAlCoordinador(int socketCoordinador, int operacion) {
	if (send(socketCoordinador, &operacion, sizeof(int), 0) < 0) {
		_exit_with_error("No se pudo enviar el resultado de la operacion al Coordinador");
	}

	if(DEBUG) {
		log_info(logger, ANSI_COLOR_BOLDWHITE"Se envio el resultado de la operacion al Coordinador");
	}
}

/******************************************************** CONEXION COORDINADOR FIN *****************************************************/

void escuchar(int socket) {
	if(listen(socket, NUMEROCLIENTES)) {
		_exit_with_error("No se puede esperar por conexiones");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se esta escuchando correctamente en el puerto"ANSI_COLOR_RESET);

}

void manejoDeClientes(int socket, cliente* socketCliente) {
	FD_ZERO(&descriptoresLectura);			//VACIO EL SET DE DESCRIPTORES
	FD_SET(socket, &descriptoresLectura); //ASIGNO EL SOCKET DE SERVIDOR AL SET

	for (int i=0; i<NUMEROCLIENTES; i++) {
		if(socketCliente[i].fd != -1 && socketCliente[i].nombre[0] != '\0') {
			FD_SET(socketCliente[i].fd, &descriptoresLectura);  //SI EL FD ES DISTINTO DE -1 LO AGREGO AL SET
		}
	}

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Se esta esperando por conexiones o mensajes de los clientes..."ANSI_COLOR_RESET);

	switch(select(fdmax+1, &descriptoresLectura, NULL, NULL, NULL)) {
		case 0: log_info(logger, "Expiro el tiempo de espera de conexion o mensaje de los clientes");
				break;

		case -1: _exit_with_error(ANSI_COLOR_BOLDRED"Fallo el manejo de clientes"ANSI_COLOR_RESET);
				break;

		default:sem_wait(&pausado);			// SI EL PLANIFICADOR ESTA PAUSADO, NO HACE NADA...
				sem_post(&pausado);				// SI PASO, LO LIBERO...

				if (FD_ISSET(socket, &descriptoresLectura)) { //ACA SE TRATA AL SOCKET SERVIDOR, SI DA TRUE ES PORQUE TIENE UN CLIENTE ESPERANDO EN COLA
					aceptarCliente(socket, socketCliente);

					if(list_is_empty(ejecutando)) {			// SI LA LISTA DE EJECUTANDO ESTA VACIA, ENTONCES MANDO ORDEN DE EJECUTAR
						ordenarProximoAEjecutar();	// PORQUE SINO MANDABA DOS EXEOR AL MISMO ESI CUANDO ESTE NO LO ESPERA
					}
				}

				for (int i=0; i<NUMEROCLIENTES; i++) {
					if (FD_ISSET(socketCliente[i].fd, &descriptoresLectura)) {
						recibirMensaje(socketCliente, i); //RECIBO EL MENSAJE, DENTRO DE LA FUNCION MANEJO ERRORES
					}
				}

				manejoDeClientes(socket, socketCliente);

				break;
	}
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

int envioIDDeESI(int socketCliente, int identificador) {
	int* identificadorESI = malloc(sizeof(int));

	*identificadorESI = identificador;

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Enviando handshake..."ANSI_COLOR_RESET);

	switch(send(socketCliente, identificadorESI, sizeof(int), 0)) {
		case -1: free(identificadorESI);
				return -1;
				break;
		default: free(identificadorESI);
				return 0;
				break;
	}
}

void aceptarCliente(int socket, cliente* socketCliente) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	log_info(logger, ANSI_COLOR_BOLDYELLOW"Un nuevo cliente requiere acceso al servidor. Procediendo a aceptar.."ANSI_COLOR_RESET);

	for (int i=0; i<NUMEROCLIENTES; i++) {			//RECORRO EL ARRAY DE CLIENTES
		if (socketCliente[i].fd == -1 && socketCliente[i].nombre[0] == '\0') {	//SI FD ES IGUAL A -1 Y NO TIENE
																				//NOMBRE ES PORQUE PUEDE OCUPAR ESE ESPACIO DEL ARRAY

			socketCliente[i].fd = accept(socket, (struct sockaddr *) &addr, &addrlen);		//ASIGNO FD AL ARRAY

			if (socketCliente[i].fd < 0) {
				_exit_with_error("No se pudo conectar el cliente");		// MANEJO DE ERRORES
			}

			if (socketCliente[i].fd > fdmax) {
				fdmax = socketCliente[i].fd;				//SI EL FD ASIGNADO ES MAYOR AL FDMAX (QUE NECESITA EL SELECT() ), LO ASIGNO AL FDMAX
			}

			switch(envioHandshake(socketCliente[i].fd)) {
				case -1: _exit_with_error("No se pudo enviar el handshake");
						break;
				case 0: log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo enviar el handshake correctamente"ANSI_COLOR_RESET);
						break;
			}

			switch(envioIDDeESI(socketCliente[i].fd, i)) {
				case -1: _exit_with_error("No se pudo enviar el ID del ESI");
						break;
				case 0: log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo enviar el ID del ESI correctamente"ANSI_COLOR_RESET);
						break;
			}

			strcpy(socketCliente[i].nombre, "ESI");		//TODOS LOS QUE SE LE CONECTAN SON ESI

			socketCliente[i].identificadorESI = i;		//LE ASIGNO EL IDENTIFICADOR AL ESI

			sem_wait(&mutexListos);
			list_add(listos, &socketCliente[i]);			//LO AGREGO A LA COLA DE LISTOS
			sem_post(&mutexListos);

			log_info(logger, ANSI_COLOR_BOLDCYAN"Se conecto un %s %d"ANSI_COLOR_RESET, socketCliente[i].nombre, socketCliente[i].identificadorESI);

			break;
		}
	}
}

void recibirHeader(int socket, header_t* header) {
	if(recv(socket, header, sizeof(header_t), MSG_WAITALL) <= 0) {		// RECIBO EL HEADER
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el header"ANSI_COLOR_RESET);
	}
}

void recibirClave(int socket, int tamanioClave, char* clave) {
	if(recv(socket, clave, tamanioClave, MSG_WAITALL) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir la clave del coordinador"ANSI_COLOR_RESET);
	}
}

void recibirIDDeESI(int socket, int* id) {
	if (recv(socket, id, sizeof(int), 0) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el ID del ESI"ANSI_COLOR_RESET);
	}
}

void recibirMensaje(cliente* socketCliente, int posicion) {
	void* buffer = malloc(7);
	int resultado_recv;

	switch(resultado_recv = recv(socketCliente[posicion].fd, buffer, 7, 0)) {

		case -1: _exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el mensaje del cliente"ANSI_COLOR_RESET);
				break;

		case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el %s %d"ANSI_COLOR_RESET, socketCliente[posicion].nombre, socketCliente[posicion].identificadorESI);
				close(socketCliente[posicion].fd); 		//CIERRO EL SOCKET
				socketCliente[posicion].fd = -1;			//LO VUELVO A SETEAR EN -1 POR SI HAY QUE VOLVER A ASIGNARLO
				break;

		default: printf(ANSI_COLOR_BOLDGREEN"Se recibio el mensaje por parte del %s %d de %d bytes y dice: %s\n"ANSI_COLOR_RESET, socketCliente[posicion].nombre, socketCliente[posicion].identificadorESI, resultado_recv, (char*) buffer);

				if (strcmp(buffer, "OPOK") == 0) {
					socketCliente[posicion].rafagaActual++;

					if (strcmp(algoritmoPlanificacion, "SJF-CD") == 0) {	//SI ES CON DESALOJO DEBO ORDENAR LA COLA CADA VEZ QUE EJECUTA UNA SENTENCIA
						socketCliente[posicion].estimacionProximaRafaga = (alfaPlanificacion / 100) * socketCliente[posicion].rafagaActual + (1 - (alfaPlanificacion / 100) ) * socketCliente[posicion].estimacionRafagaActual;

						if(DEBUG) {
							printf(ANSI_COLOR_BOLDWHITE"ESI %d - Estimacion Proxima Rafaga: %f - Estimacion Rafaga Anterior/Actual: %f \n"ANSI_COLOR_RESET, socketCliente[posicion].identificadorESI,socketCliente[posicion].estimacionProximaRafaga, socketCliente[posicion].estimacionRafagaActual);
						}

						socketCliente[posicion].estimacionRafagaActual = socketCliente[posicion].estimacionProximaRafaga;

						if (list_size(listos) >= 2) {			//SI HAY MAS DOS ESIS EN LISTOS ORDENO
							ordenarColaDeListosPorSJF();
						}

						verificarDesalojoPorSJF(&socketCliente[posicion]);	// Y VERIFICO SI HAY QUE DESALOJAR AL ACTUAL
					}

					if (strcmp(algoritmoPlanificacion, "HRRN") == 0) {
						socketCliente[posicion].estimacionProximaRafaga = (alfaPlanificacion / 100) * socketCliente[posicion].rafagaActual + (1 - (alfaPlanificacion / 100) ) * socketCliente[posicion].estimacionRafagaActual;
						socketCliente[posicion].estimacionRafagaActual = socketCliente[posicion].estimacionProximaRafaga;
						socketCliente[posicion].tiempoDeEspera = 0; 	// LO REINICIO CADA VEZ QUE DEVUELVE OPOK (DEBERIA SER SOLO UNA VEZ CUANDO ENTRA A EJECUTAR PERO BUE)

						sem_wait(&mutexListos);
						list_iterate(listos, (void *) sumarUnoAlWaitingTime);
						sem_post(&mutexListos);
					}

					ordenarProximoAEjecutar();
				}

				if (strcmp(buffer, "EXEEND") == 0) {
					sem_wait(&mutexEjecutando);
					list_remove(ejecutando, 0);
					sem_post(&mutexEjecutando);

					sem_wait(&mutexFinalizados);
					list_add(finalizados, &socketCliente[posicion]);
					sem_post(&mutexFinalizados);

					if (list_size(listos) >= 2) {			//SI EN LISTOS HAY MAS DE 2 ESIS ORDENO...
						if(strncmp(algoritmoPlanificacion, "SJF", 3) == 0) { // SI ES SJF
							ordenarColaDeListosPorSJF(&socketCliente[posicion]);
						}

						else if (strcmp(algoritmoPlanificacion, "HRRN") == 0) { //SI ES HRRN
							sem_wait(&mutexListos);
							list_iterate(listos, (void*) calcularResponseRatio);	// CALCULO LA TASA DE RESPUESTA DE TODOS LOS ESIS LISTOS
							sem_post(&mutexListos);
							ordenarColaDeListosPorHRRN();
						}
					}

					ordenarProximoAEjecutar();	//ENVIO ORDEN DE EJECUCION SI HAY LISTOS PARA EJECUTAR
				}
				break;

	}

	free(buffer);
}

void ordenarProximoAEjecutar() {
	char* ordenEjecucion = "EXEOR";
	cliente* esiProximoAEjecutar;

	if(list_is_empty(listos) && list_is_empty(ejecutando)) {		// SI LAS DOS LISTAS ESTAN VACIAS, NADIE VA A EJECUTAR
		printf(ANSI_COLOR_BOLDRED"\nNo hay ESIs para ejecutar\n\n"ANSI_COLOR_RESET);
	} else if (!list_is_empty(ejecutando)){							// SI EJECUTANDO NO ESTA VACIA, DEBERIA SEGUIR EJECUTANDO EL...
		esiProximoAEjecutar = list_get(ejecutando, 0);

		enviarOrdenDeEjecucion(esiProximoAEjecutar, ordenEjecucion);
	} else {
		esiProximoAEjecutar = getPrimerESIListo();

		enviarOrdenDeEjecucion(esiProximoAEjecutar, ordenEjecucion);
	}
}

cliente* getPrimerESIListo() {
	cliente* cliente;

	cliente = list_get(listos, 0);					//TOMO EL PRIMERO DE LA LISTA DE LISTOS
	sem_wait(&mutexListos);
	list_remove(listos, 0);							//LO SACO PORQUE PASA A EJECUTAR
	sem_post(&mutexListos);

	sem_wait(&mutexEjecutando);
	list_add(ejecutando, cliente);					//LO AGREGO A LA LISTA DE EJECUTANDO
	sem_post(&mutexEjecutando);
	return cliente;
}

void ordenarColaDeListosPorSJF() {
	sem_wait(&mutexListos);
	list_sort(listos, (void*) comparadorRafaga);
	sem_post(&mutexListos);

	if(DEBUG) {
		/*************************** PARA VER COMO QUEDO ORDENADA LA LISTA ***********************/

		printf(ANSI_COLOR_BOLDWHITE"\n\n\n--------------------SE ORDENO LA COLA DE LISTOS-----------------------\n\n\n"ANSI_COLOR_RESET);

		for (int i=0; i<list_size(listos); i++) {
			struct Cliente* esi = list_get(listos, i);
			printf(ANSI_COLOR_BOLDWHITE"ESI %d ------ Rafaga: %f\n"ANSI_COLOR_RESET, esi->identificadorESI, esi->estimacionProximaRafaga);
		}

		printf("\n\n\n");

		/*****************************************************************************************/
	}
}

void verificarDesalojoPorSJF(cliente* ESIEjecutando) {
	struct Cliente* primerESIListo;

	if(!list_is_empty(listos) && !list_is_empty(ejecutando)) {
		primerESIListo = list_get(listos, 0);			//AGARRO EL QUE AHORA ESTA PRIMERO EN LA LISTA DE LISTOS

		if (primerESIListo->estimacionProximaRafaga < ESIEjecutando->estimacionProximaRafaga) {	// SI LA RAFAGA ESTIMADA DEL ESI LISTO ES MENOR AL QUE EJECUTA
			sem_wait(&mutexEjecutando);
			list_remove(ejecutando, 0);			// DESALOJO AL ESI EJECUTANDO
			sem_post(&mutexEjecutando);

			sem_wait(&mutexListos);
			list_add(listos, ESIEjecutando);	// LO AGREGO AL FINAL DE LA LISTA DE LISTOS
			sem_post(&mutexListos);
			ESIEjecutando->rafagaActual = 0;	// SE RESETEA LA RAFAGA ACTUAL (PORQUE FUE DESALOJADO)

			sem_wait(&mutexEjecutando);
			list_add(ejecutando, primerESIListo);
			sem_post(&mutexEjecutando);
		}
	}
}

void ordenarColaDeListosPorHRRN() {
	sem_wait(&mutexListos);
	list_sort(listos, (void*) comparadorResponseRatio);
	sem_post(&mutexListos);

	if(DEBUG) {
		/*************************** PARA VER COMO QUEDO ORDENADA LA LISTA ***********************/

		printf(ANSI_COLOR_BOLDWHITE"\n\n\n--------------------SE ORDENO LA COLA DE LISTOS-----------------------\n\n\n"ANSI_COLOR_RESET);

		for (int i=0; i<list_size(listos); i++) {
			struct Cliente* esi = list_get(listos, i);
			printf(ANSI_COLOR_BOLDWHITE"ESI %d ------ Response ratio: %f\n"ANSI_COLOR_RESET, esi->identificadorESI, esi->tasaDeRespuesta);
		}

		printf("\n\n\n");

		/*****************************************************************************************/
	}
}

void enviarOrdenDeEjecucion(cliente* esiProximoAEjecutar, char* ordenEjecucion) {
	if(send(esiProximoAEjecutar->fd, ordenEjecucion, strlen(ordenEjecucion)+1, 0) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"Fallo el envio de orden de ejecucion al ESI"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDWHITE"Se envio orden de ejecucion al %s %d"ANSI_COLOR_RESET, esiProximoAEjecutar->nombre, esiProximoAEjecutar->identificadorESI);
}

int comparadorRafaga(cliente* cliente, struct Cliente* cliente2) {
	if(cliente->estimacionProximaRafaga <= cliente2->estimacionProximaRafaga)
		return 1;
	else
		return 0;
}

int comparadorResponseRatio(cliente* cliente, struct Cliente* cliente2) {
	if(cliente->tasaDeRespuesta <= cliente2->tasaDeRespuesta)
		return 1;
	else
		return 0;
}

void sumarUnoAlWaitingTime(cliente* cliente) {
	cliente->tiempoDeEspera += 1;
}

void calcularResponseRatio(cliente* cliente) {
	cliente->tasaDeRespuesta = (cliente->tiempoDeEspera + cliente->estimacionProximaRafaga) / cliente->estimacionProximaRafaga;
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
	crearDiccionarioDeClaves();
	setearConfigEnVariables();
	setearListaDeEstados();

	/************************************** MUESTRO CLAVES INICIALMENTE BLOQUEADAS *************************/

	char* IDEsi;

	IDEsi = dictionary_get(diccionarioClaves, "futbol:messi");

	for (int i=0; i<dictionary_size(diccionarioClaves); i++) {
		printf("%s \n", IDEsi);
	}

	/************************************** CONEXION CON COORDINADOR **********************************/

	pthread_t threadCliente;

	if(pthread_create(&threadCliente, NULL, (void *)conectarConCoordinador, NULL) != 0) {
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo coordinador"ANSI_COLOR_RESET);
		exit(-1);
	}else{
		log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo coordinador"ANSI_COLOR_RESET);
	}

	sleep(1);

	/************************************** CONEXION CON CONSOLA **********************************/

	pthread_t threadConsola;

	if(pthread_create(&threadConsola, NULL, (void *)ejecutar_consola, NULL) != 0) {
		log_error(logger, "No se pudo crear el hilo consola");
		exit(-1);
	}else{
		log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo consola"ANSI_COLOR_RESET);
	}

	pthread_detach(threadCliente);
	pthread_detach(threadConsola);

	listenSocket = conectarSocketYReservarPuerto();

	escuchar(listenSocket);

	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i].fd = -1;				//INICIALIZO EL ARRAY DE FDS EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
		socketCliente[i].estimacionProximaRafaga = estimacionInicial;	//ESTIMACION INICIAL POR DEFAULT
		socketCliente[i].rafagaActual = 0;
		socketCliente[i].estimacionRafagaActual = estimacionInicial;
		socketCliente[i].tasaDeRespuesta = 0;
	}

	manejoDeClientes(listenSocket, socketCliente);

	close(listenSocket);

	return EXIT_SUCCESS;
}
