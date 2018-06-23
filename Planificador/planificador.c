/******** SERVIDOR PLANIFICADOR *********/
#include "consola.h"
#include "planificador.h"

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
	sem_init(&mutexDiccionarioClaves, 0, 1);
	sem_init(&esisListos, 0, 0);
	sem_init(&desalojoComandoBloquear, 0, 1);
	sem_init(&desalojoComandoKill, 0, 1);

	/************************ SETEO CLAVES BLOQUEADAS ******************/

	char* claves = config_get_string_value(config, "Claves inicialmente bloqueadas");
	int* IDSistema = malloc(sizeof(int));
	*IDSistema = -1;

	if (claves != NULL) {
		char** clavesSeparadas = string_split(claves, ",");

		for (int i=0; clavesSeparadas[i] != NULL; i++) {		// SIGO HASTA QUE SEA NULL EL ARRAY
			sem_wait(&mutexDiccionarioClaves);
			dictionary_put(diccionarioClaves, clavesSeparadas[i], IDSistema);	// AGREGO LA CLAVE Y QUIEN LA TOMA ES EL "SISTEMA"
			sem_post(&mutexDiccionarioClaves);
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
	int fdmaxCoordinador;
	header_t* buffer_header = malloc(sizeof(header_t));
	int* IDESI = malloc(sizeof(int));
	char* clave;
	cliente* ESI;

	while(1) {
		FD_ZERO(&descriptorCoordinador);
		FD_SET(socket, &descriptorCoordinador);
		fdmaxCoordinador = socket;

		select(fdmaxCoordinador + 1, &descriptorCoordinador, NULL, NULL, NULL);

		if (FD_ISSET(socket, &descriptorCoordinador)) {

			recibirHeader(socket, buffer_header);

			clave = malloc(buffer_header->tamanioClave);

			recibirClave(socket, buffer_header->tamanioClave, clave);

			recibirIDDeESI(socket, IDESI);

			int* idEsiParaDiccionario = malloc(sizeof(int));

			*idEsiParaDiccionario = *IDESI;

			ESI = buscarESI(IDESI);

			switch(buffer_header->codigoOperacion) {
			case 0: // OPERACION GET
				if (dictionary_has_key(diccionarioClaves, clave)) {

					sem_wait(&mutexDiccionarioClaves);
					int* IDEsiQueTieneLaClaveTomada = dictionary_get(diccionarioClaves, clave);
					sem_post(&mutexDiccionarioClaves);

					log_error(logger, ANSI_COLOR_BOLDRED"La clave %s ya estaba tomada por el ESI %d. Se bloqueo al ESI %d"ANSI_COLOR_RESET, clave, *IDEsiQueTieneLaClaveTomada, *idEsiParaDiccionario);

					bloquearESI(clave, IDESI);

					informarAlCoordinador(socket, 0);	// 0 PORQUE SALIO MAL
				} else {

					sem_wait(&mutexDiccionarioClaves);
					dictionary_put(diccionarioClaves, clave, idEsiParaDiccionario);
					sem_post(&mutexDiccionarioClaves);

					log_info(logger, ANSI_COLOR_BOLDCYAN"El ESI %d tomo efectivamente la clave %s"ANSI_COLOR_RESET, *idEsiParaDiccionario, clave);

					informarAlCoordinador(socket, 1); // 1 PORQUE SALIO BIEN

					recibirMensaje(ESI);
				}
				break;
			case 1: //OPERACION SET

				// VERIFICAR SI YO TENGO QUE VER SI EL SET TIRA ERROR POR CLAVE NO IDENTIFICADA O EL COORDINADOR

				recibirMensaje(ESI);

				break;
			case 2: //OPERACION STORE
				if (dictionary_has_key(diccionarioClaves, clave)) {
					sem_wait(&mutexDiccionarioClaves);
					int* IDEsiQueTieneLaClaveTomada = dictionary_get(diccionarioClaves, clave);
					sem_post(&mutexDiccionarioClaves);

					if(*idEsiParaDiccionario == *IDEsiQueTieneLaClaveTomada) {	// SI EL QUE QUIERE STOREAR ES EL MISMO QUE LA TIENE STOREADA, OK.
						sem_wait(&mutexDiccionarioClaves);
						int* value = dictionary_remove(diccionarioClaves, clave);
						sem_post(&mutexDiccionarioClaves);
						free(value);	// NO LO NECESITO MAS

						log_info(logger, ANSI_COLOR_BOLDCYAN"Se removio la clave %s tomada por el ESI %d"ANSI_COLOR_RESET, clave, *idEsiParaDiccionario);

						informarAlCoordinador(socket, 1); // 1 PORQUE SALIO BIEN

						// DESBLOQUEO AL ESI QUE ESTA ESPERANDO POR ESTE RECURSO, SI LO HUBIESE

						desbloquearESI(clave);

						recibirMensaje(ESI);

					} else {		// DEBO ABORTAR AL ESI POR STOREAR UNA CLAVE QUE NO ES DE EL
						abortarESI(IDESI);

						log_error(logger, ANSI_COLOR_BOLDRED"Se aborto el ESI %d por querer hacer STORE de la clave %s que no es de el"ANSI_COLOR_RESET, *idEsiParaDiccionario, clave);

						free(idEsiParaDiccionario);	// NO LA NECESITO

						informarAlCoordinador(socket, 0); // 0 PORQUE SALIO MAL

						ordenarProximoAEjecutar();	// ORDENO PROXIMO A EJECUTAR YA QUE ABORTE A UN ESI...
					}
				} else {		// SI LA CLAVE NO ESTA EN EL DICCIONARIO...
					abortarESI(IDESI);

					log_error(logger, ANSI_COLOR_BOLDRED"Se aborto el ESI %d por querer hacer STORE de la clave %s inexistente"ANSI_COLOR_RESET, *idEsiParaDiccionario, clave);

					free(idEsiParaDiccionario);	// NO LA NECESITO

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
	if( *(int*)ESI == *ESIABuscarEnDiccionario) {
		log_info(logger, ANSI_COLOR_BOLDMAGENTA"Se elimino la clave %s tomada por el ESI %d ya que fue finalizado"ANSI_COLOR_RESET, clave, *(int*) ESI);
		int* value = dictionary_remove(diccionarioClaves, clave);
		free(value);
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

void abortarESI(int* IDESI) {
	ESIABuscarEnDiccionario = malloc(sizeof(int));

	*ESIABuscarEnDiccionario = *IDESI;	// SETEO LA VARIABLE GLOBAL

	sem_wait(&mutexDiccionarioClaves);
	dictionary_iterator(diccionarioClaves, (void*) eliminarClavesTomadasPorEsiFinalizado); // REMUEVO LAS CLAVES TOMADAS POR EL ESI A FINALIZAR
	sem_post(&mutexDiccionarioClaves);

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
		ESI->estimacionRafagaActual = ESI->estimacionProximaRafaga;		// AHORA LA RAFAGA ANTERIOR PASA A SER LA ESTIMADA PORQUE SE DESALOJO
		ESI->rafagaActual = 0; 	// LO SETEO EN 0 PORQUE FUE "DESALOJADO" PORQUE SE BLOQUEO
	}

	if (strcmp(algoritmoPlanificacion, "HRRN") == 0) {
		ESI->estimacionProximaRafaga = (alfaPlanificacion / 100) * ESI->rafagaActual + (1 - (alfaPlanificacion / 100) ) * ESI->estimacionRafagaActual;
		ESI->estimacionRafagaActual = ESI->estimacionProximaRafaga;		// AHORA LA RAFAGA ANTERIOR PASA A SER LA ESTIMADA PORQUE SE DESALOJO
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

			log_info(logger, ANSI_COLOR_BOLDCYAN"Se desbloqueo al ESI %d ya que se libero la clave %s"ANSI_COLOR_RESET, primerESIBloqueadoEsperandoPorClave->identificadorESI, clave);

			sem_wait(&mutexListos);
			list_add(listos, primerESIBloqueadoEsperandoPorClave);	// LO MANDO A LISTOS
			sem_post(&mutexListos);
		}
	}

	if(list_is_empty(ejecutando)) {
		sem_post(&esisListos); // LE AVISO AL HILO DE PLANIFICACION QUE EJECUTE. PORQUE SI ESTABA PAUSADO Y DESBLOQUEABA POR CONSOLA, IGUAL PLANIFICABA
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

void manejoDeClientes() {
	log_info(logger, ANSI_COLOR_BOLDYELLOW"Se esta esperando por conexiones de los clientes..."ANSI_COLOR_RESET);

	aceptarCliente(listenSocket);

	sem_post(&esisListos);
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

void aceptarCliente(int socket) {
	struct sockaddr_in addr;			// Esta estructura contendra los datos de la conexion del cliente. IP, puerto, etc.
	socklen_t addrlen = sizeof(addr);

	for (int i=0; i<NUMEROCLIENTES; i++) {			//RECORRO EL ARRAY DE CLIENTES
		if (socketCliente[i].fd == -1 && socketCliente[i].nombre[0] == '\0') {	//SI FD ES IGUAL A -1 Y NO TIENE
																				//NOMBRE ES PORQUE PUEDE OCUPAR ESE ESPACIO DEL ARRAY

			socketCliente[i].fd = accept(socket, (struct sockaddr *) &addr, &addrlen);		//ASIGNO FD AL ARRAY

			if (socketCliente[i].fd < 0) {
				_exit_with_error("No se pudo conectar el cliente");		// MANEJO DE ERRORES
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

void planificar() {
	while(1) {
		sem_wait(&esisListos);
		sem_wait(&pausado);
		sem_post(&pausado);

		if(list_is_empty(ejecutando)) {
			ordenarProximoAEjecutar();
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

void recibirMensaje(cliente* ESI) {
	void* buffer = malloc(7);
	int resultado_recv;

	switch(resultado_recv = recv(ESI->fd, buffer, 7, 0)) {

		case -1: _exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el mensaje del cliente"ANSI_COLOR_RESET);
				break;

		case 0: log_info(logger, ANSI_COLOR_BOLDRED"Se desconecto el %s %d"ANSI_COLOR_RESET, ESI->nombre, ESI->identificadorESI);
				close(ESI->fd); 		//CIERRO EL SOCKET
				ESI->fd = -1;			//LO VUELVO A SETEAR EN -1
				break;

		default: printf(ANSI_COLOR_BOLDGREEN"Se recibio el mensaje por parte del %s %d de %d bytes y dice: %s\n"ANSI_COLOR_RESET, ESI->nombre, ESI->identificadorESI, resultado_recv, (char*) buffer);

				if (strcmp(buffer, "OPOK") == 0) {
					ESI->rafagaActual++;

					if (strcmp(algoritmoPlanificacion, "SJF-CD") == 0) {	//SI ES CON DESALOJO DEBO ORDENAR LA COLA CADA VEZ QUE EJECUTA UNA SENTENCIA
						ESI->estimacionProximaRafaga = (alfaPlanificacion / 100) * ESI->rafagaActual + (1 - (alfaPlanificacion / 100) ) * ESI->estimacionRafagaActual;

						if(DEBUG) {
							printf(ANSI_COLOR_BOLDWHITE"ESI %d - Estimacion Proxima Rafaga: %f - Estimacion Rafaga Anterior/Actual: %f \n"ANSI_COLOR_RESET, ESI->identificadorESI, ESI->estimacionProximaRafaga, ESI->estimacionRafagaActual);
						}

						if (list_size(listos) >= 2) {			//SI HAY MAS DOS ESIS EN LISTOS ORDENO
							ordenarColaDeListosPorSJF();
						}

						verificarDesalojoPorSJF(ESI);	// Y VERIFICO SI HAY QUE DESALOJAR AL ACTUAL
					}

					if (strcmp(algoritmoPlanificacion, "HRRN") == 0) {
						ESI->estimacionProximaRafaga = (alfaPlanificacion / 100) * ESI->rafagaActual + (1 - (alfaPlanificacion / 100) ) * ESI->estimacionRafagaActual;
						ESI->tiempoDeEspera = 0; 	// LO REINICIO CADA VEZ QUE DEVUELVE OPOK (DEBERIA SER SOLO UNA VEZ CUANDO ENTRA A EJECUTAR PERO BUE)

						sem_wait(&mutexListos);
						list_iterate(listos, (void *) sumarUnoAlWaitingTime);
						sem_post(&mutexListos);
					}

					if(ESI->desalojoPorComandoBloquear) {
						sem_wait(&desalojoComandoBloquear);
						ESI->desalojoPorComandoBloquear = 0;		// LO VUELVO A SETEAR EN 0
						sem_post(&desalojoComandoBloquear);

						sem_wait(&mutexEjecutando);
						list_remove(ejecutando, 0);
						sem_post(&mutexEjecutando);

						sem_wait(&mutexBloqueados);
						list_add(bloqueados, ESI);
						sem_post(&mutexBloqueados);
					}

					if(ESI->desalojoPorComandoKill) {
						sem_wait(&desalojoComandoKill);
						ESI->desalojoPorComandoKill = 0;
						sem_post(&desalojoComandoKill);

						sem_wait(&mutexEjecutando);
						list_remove(ejecutando, 0);
						sem_post(&mutexEjecutando);

						sem_wait(&mutexFinalizados);
						list_add(finalizados, ESI);
						sem_post(&mutexFinalizados);

						ESIABuscarEnDiccionario = malloc(sizeof(int));
						*ESIABuscarEnDiccionario = ESI->identificadorESI;	// SETEO LA VARIABLE GLOBAL

						sem_wait(&mutexDiccionarioClaves);
						dictionary_iterator(diccionarioClaves, (void*) eliminarClavesTomadasPorEsiFinalizado); // REMUEVO LAS CLAVES TOMADAS POR EL ESI A FINALIZAR
						sem_post(&mutexDiccionarioClaves);

						free(ESIABuscarEnDiccionario);
					}

					ordenarProximoAEjecutar();
				}

				if (strcmp(buffer, "EXEEND") == 0) {
					sem_wait(&mutexEjecutando);
					list_remove(ejecutando, 0);
					sem_post(&mutexEjecutando);

					sem_wait(&mutexFinalizados);
					list_add(finalizados, ESI);
					sem_post(&mutexFinalizados);

					/*************************** LIBERO LAS CLAVES TOMADAS DEL ESI ************************/

					ESIABuscarEnDiccionario = malloc(sizeof(int));

					*ESIABuscarEnDiccionario = ESI->identificadorESI;	// SETEO LA VARIABLE GLOBAL

					sem_wait(&mutexDiccionarioClaves);
					dictionary_iterator(diccionarioClaves, (void*) eliminarClavesTomadasPorEsiFinalizado); // REMUEVO LAS CLAVES TOMADAS POR EL ESI A FINALIZAR
					sem_post(&mutexDiccionarioClaves);

					free(ESIABuscarEnDiccionario);

					/*************************************************************************************/

					close(ESI->fd);
					ESI->fd = -1;	// LO SETEO EN -1 PORQUE YA SE DESCONECTO

					if (list_size(listos) >= 2) {			//SI EN LISTOS HAY MAS DE 2 ESIS ORDENO...
						if(strncmp(algoritmoPlanificacion, "SJF", 3) == 0) { // SI ES SJF
							ordenarColaDeListosPorSJF(ESI);
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

	sem_wait(&pausado);	// SI ESTA PAUSADO NO EJECUTO
	sem_post(&pausado);

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
			list_remove(listos, 0);		// Y SACO DE LISTOS AL QUE VA A EJECUTAR
			sem_post(&mutexListos);
			ESIEjecutando->rafagaActual = 0;	// SE RESETEA LA RAFAGA ACTUAL (PORQUE FUE DESALOJADO)
			ESIEjecutando->estimacionRafagaActual = ESIEjecutando->estimacionProximaRafaga;		// AHORA LA RAFAGA ANTERIOR PASA A SER LA ESTIMADA PORQUE SE DESALOJO

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
			printf(ANSI_COLOR_BOLDWHITE"ESI %d ------ Response ratio: %f - Tiempo de espera: %d - Estimacion Proxima Rafaga: %f\n"ANSI_COLOR_RESET, esi->identificadorESI, esi->tasaDeRespuesta, esi->tiempoDeEspera, esi->estimacionProximaRafaga);
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
	if(cliente->tasaDeRespuesta >= cliente2->tasaDeRespuesta)
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

int esiEstaEjecutando(cliente* ESI) {
	if (!list_is_empty(ejecutando)) {
		cliente* ESIEjecutando = list_get(ejecutando, 0);

		if(ESIEjecutando->identificadorESI == ESI->identificadorESI) {	// SI EL ESI ESTABA EJECUTANDO, TENGO QUE DESALOJARLO Y ORDENAR A OTRO A EJECUTAR
			return 1;
		} else {
			return 0;
		}
	}

	return 0;
}

int esiEstaEnListos(cliente* ESI) {
	for (int i=0; i<list_size(listos); i++) {
		cliente* ESIListo = list_get(listos, i);

		if (ESIListo->identificadorESI == ESI->identificadorESI) {
			return 1;
		}
	}

	return 0;
}

int esiEstaEnBloqueados(cliente* ESI) {
	for (int i=0; i<list_size(bloqueados); i++) {
		cliente* ESIBloqueado = list_get(bloqueados, i);

		if (ESIBloqueado->identificadorESI == ESI->identificadorESI) {
			return 1;
		}
	}

	return 0;
}

int hayEsisBloqueadosEsperandoPor(char* clave) {
	for(int i=0; i<NUMEROCLIENTES; i++) {
		if (strcmp(socketCliente[i].recursoSolicitado, clave) == 0) {
			return 1;
		}
	}

	return 0;
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

	/************************************** CONEXION CON COORDINADOR **********************************/

	pthread_t threadCliente;

	if(pthread_create(&threadCliente, NULL, (void *)conectarConCoordinador, NULL) != 0) {
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo coordinador"ANSI_COLOR_RESET);
		exit(-1);
	}else{
		log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo coordinador"ANSI_COLOR_RESET);
	}

	sleep(1);

	/************************************** HILO CONSOLA **********************************/

	pthread_t threadConsola;

	if(pthread_create(&threadConsola, NULL, (void *)ejecutar_consola, NULL) != 0) {
		log_error(logger, "No se pudo crear el hilo consola");
		exit(-1);
	}else{
		log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo consola"ANSI_COLOR_RESET);
	}

	/************************************** HILO PLANIFICACION **********************************/

	pthread_t threadPlanificacion;

	if(pthread_create(&threadPlanificacion, NULL, (void *)planificar, NULL) != 0) {
		log_error(logger, "No se pudo crear el hilo de planificacion");
		exit(-1);
	}else{
		log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo de planificacion"ANSI_COLOR_RESET);
	}

	pthread_detach(threadCliente);
	pthread_detach(threadConsola);
	pthread_detach(threadPlanificacion);

	/***************************************************************************************/

	listenSocket = conectarSocketYReservarPuerto();

	escuchar(listenSocket);

	for (int i=0; i<NUMEROCLIENTES; i++) {
		socketCliente[i].fd = -1;				//INICIALIZO EL ARRAY DE FDS EN -1 PORQUE 0,1 Y 2 YA ESTAN RESERVADOS
		socketCliente[i].estimacionProximaRafaga = estimacionInicial;	//ESTIMACION INICIAL POR DEFAULT
		socketCliente[i].rafagaActual = 0;
		socketCliente[i].estimacionRafagaActual = 0;
		socketCliente[i].tasaDeRespuesta = 0;
		socketCliente[i].desalojoPorComandoBloquear = 0;
		socketCliente[i].desalojoPorComandoKill = 0;
	}

	while(1) {
		manejoDeClientes();
	}

	close(listenSocket);

	return EXIT_SUCCESS;
}
