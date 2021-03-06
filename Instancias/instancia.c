/******** CLIENTE INSTANCIAS *********/

#include "instancia.h"

/* CONEXIONES */
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
	//TAMANIOENTRADA = config_get_int_value(config, "Tamanio de Entrada");
	//CANTIDADENTRADAS  = config_get_int_value(config, "Cantidad de Entradas");
	IDENTIFICADORINSTANCIA = config_get_int_value(config, "ID de Instancia");
	sem_init(&mutexTablaDeEntradas, 0, 1);				// INICIALIZO EL SEMAFORO DEL MUTEX DE LA TABLA DE ENTRADAS
	sem_init(&mutexOperaciones, 0, 1);
	STORAGEAPUNTADO = 0;		// LA INICIALIZO
	DUMP = 0;		// INICIALIZO QUE EL DUMP NO ESTA EN CURSO
	LEVANTODEDISCO = 0;		// INICIALIZO QUE NO LEVANTO DE DISCO
	COMPACTARPORPEDIDO = 0;	// INICIALIZO QUE NO COMPACTO POR PEDIDO

//    storageFijo = malloc(CANTIDADENTRADAS * TAMANIOENTRADA); // CREO VECTOR
//    inicializarStorage();
//    storage = storageFijo; 			// SIEMPRE TOCAR EL STORAGE AUXILIAR (STORAGE), EL OTRO NO
}

void inicializarStorage() {
	for (int i=0; i<CANTIDADENTRADAS; i++) {
		storage = buscarEnStorage(i);
		strcpy(storage, "");
	}

//	storage = buscarEnStorage(0);
//	strcpy(storage, "AA");
//	storage = buscarEnStorage(3);
//	strcpy(storage, "AA");
//	storage = buscarEnStorage(5);
//	strcpy(storage, "AA");
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
			sem_wait(&mutexOperaciones);
			recibirInstruccion(socket);
			sem_post(&mutexOperaciones);
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
	int* instanciaNueva = malloc(sizeof(int));
	int* cantidadClaves = malloc(sizeof(int));
	int* tamanioClave = malloc(sizeof(int));
	char* clave;

	if (send(socket, identificador, strlen(identificador)+1, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el identificador"ANSI_COLOR_RESET);
	}

	/* Recibir cantidad y tamaño de entradas */
	if(recv(socket, &TAMANIOENTRADA, sizeof(int), 0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir el tamanio de entrada"ANSI_COLOR_RESET);
	}

	if(recv(socket, &CANTIDADENTRADAS, sizeof(int), 0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir la cantidad de entradas"ANSI_COLOR_RESET);
	}

	storageFijo = malloc(CANTIDADENTRADAS * TAMANIOENTRADA); // CREO VECTOR
	inicializarStorage();
	storage = storageFijo; 			// SIEMPRE TOCAR EL STORAGE AUXILIAR (STORAGE), EL OTRO NO

	if (send(socket, &IDENTIFICADORINSTANCIA, sizeof(int), 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar ID de Instancia"ANSI_COLOR_RESET);
	}
	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio correctamente el identificador e instancia"ANSI_COLOR_RESET);

	if(recv(socket, instanciaNueva, sizeof(int), 0) < 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir si es instancia nueva o vieja"ANSI_COLOR_RESET);
	}

	if(*instanciaNueva == 1){
		if(recv(socket, cantidadClaves, sizeof(int), 0) < 0){
			_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir la cantidad de claves"ANSI_COLOR_RESET);
		}

		while(*cantidadClaves){
			if(recv(socket, tamanioClave, sizeof(int), 0) < 0){
				_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir el tamanio de la clave"ANSI_COLOR_RESET);
			}

			clave = malloc(*tamanioClave);

			if(recv(socket, clave, *tamanioClave, 0) < 0){
				_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir la clave"ANSI_COLOR_RESET);
			}

			log_info(logger,ANSI_COLOR_BOLDYELLOW"Se recibio la clave %s"ANSI_COLOR_RESET, clave);

			levantarClaveDeDisco(clave);

			(*cantidadClaves)--;

			free(clave);
		}
	}

	free(instanciaNueva);
	free(cantidadClaves);
	free(tamanioClave);
}

void levantarClaveDeDisco(char* clave) {
	char* directorio = malloc(strlen(PUNTOMONTAJE) + strlen(clave) + 1);
	char* valor;
	size_t len = 0;
	FILE* f;
	LEVANTODEDISCO = 1;

	strcpy(directorio, PUNTOMONTAJE);
	strcat(directorio, clave);

	if ((f = fopen(directorio, "r"))) {
		getline(&valor, &len, f);		// COPIO EL VALOR
		fclose(f);
	} else {
		valor = malloc(strlen("def") + 1);
		strcpy(valor, "def");
	}

	set(clave, valor);			// HAGO EL SET DE CADA CLAVE

	LEVANTODEDISCO = 0;
}


void pipeHandler() {
	printf(ANSI_COLOR_BOLDRED"***********************************LA INSTANCIA SE CERRO***********************************\n"ANSI_COLOR_RESET);
	printf(ANSI_COLOR_BOLDRED"Liberando memoria...\n"ANSI_COLOR_RESET);

	free(storageFijo);
	exit(1);
}

/* Recibir operacion de Coordinador */
void recibirInstruccion(int socket){

	header_t* buffer_header = malloc(sizeof(header_t));
	char* bufferClave;
	char* bufferValor;
	int32_t* tamanioValor = malloc(sizeof(int32_t));
	entrada_t* entrada;
	int* tamanioValorStatus = malloc(sizeof(int));
	char* valorStatus;
	char* valor;
	int* entradasLibresCoordinador;

	if(recv(socket, buffer_header, sizeof(header_t), MSG_WAITALL)< 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir el Header"ANSI_COLOR_RESET);
	}

	bufferClave = malloc(buffer_header->tamanioClave);

	switch(buffer_header->codigoOperacion){
	case 1: /* SET */

		/* Recibo clave y valor */
		recibirClave(socket, buffer_header, bufferClave);
		recibirTamanioValor(socket, tamanioValor);
		bufferValor = malloc(*tamanioValor);
		recibirValor(socket, tamanioValor, bufferValor);

		set(bufferClave, bufferValor);

		mostrarStorage();

		free(bufferValor);

		break;
	case 2: /* STORE */
		/* Recibo clave */
		recibirClave(socket, buffer_header, bufferClave);

		store(bufferClave);
		break;
	case 3: /* Status */
		recibirClave(socket, buffer_header, bufferClave);

		enviarHeader(socketCoordinador, 3, strlen(bufferClave) + 1);

		if((entrada = buscarEnTablaDeEntradas(bufferClave)) != NULL) {
			valor = buscarEnStorage(entrada->numero);
			*tamanioValorStatus = entrada->tamanio_valor;
			valorStatus = malloc(*tamanioValorStatus);
			memcpy(valorStatus, valor, entrada->tamanio_valor-1);		// NO TIENE \0
			valorStatus[entrada->tamanio_valor] = '\0';					// SE LO AGREGO

//			printf(ANSI_COLOR_BOLDWHITE"VALOR STATUS %s\n"ANSI_COLOR_RESET, valorStatus);

			enviarTamanioValor(socket, tamanioValorStatus);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio el tamanio del valor al coordinador"ANSI_COLOR_RESET);
			enviarValor(socket, *tamanioValorStatus, valorStatus);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio el valor al coordinador"ANSI_COLOR_RESET);
			free(valorStatus);
		}else{
			*tamanioValorStatus = -1;
			enviarTamanioValor(socket, tamanioValorStatus);
			log_info(logger, "Se envio el tamanio del valor al coordinador");
		}

		break;
	case 7: // SE DEBE COMPACTAR
		COMPACTARPORPEDIDO = 1;
		compactar();
		COMPACTARPORPEDIDO = 0;
		break;
	case 8: /* NOTIFICAR ENTRADAS LIBRES */
		entradasLibresCoordinador = malloc(sizeof(int));

		*entradasLibresCoordinador = entradasLibres();

		enviarHeader(socketCoordinador, 8, -1);

		if (send(socket, entradasLibresCoordinador, sizeof(int), 0) < 0) {
			_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar la cantidad de entradas libres"ANSI_COLOR_RESET);
		}

		log_info(logger,ANSI_COLOR_BOLDYELLOW"Entradas libres: %d"ANSI_COLOR_RESET, *entradasLibresCoordinador);

		free(entradasLibresCoordinador);
		break;
	default:
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No existe el codigo de operacion"ANSI_COLOR_RESET);
		break;
	}

	free(buffer_header);
	free(tamanioValor);
	free(tamanioValorStatus);
	free(bufferClave);
}

void enviarHeader(int socket, int codigoOP, int tamanioClave) {
	header_t* header = malloc(sizeof(header_t));

	header->codigoOperacion = codigoOP;
	header->tamanioClave = tamanioClave;

	if (send(socket, header, sizeof(header_t), 0) < 0) {
		_exit_with_error(socketCoordinador, ANSI_COLOR_BOLDRED"No se pudo enviar el header de operacion OK al Coordinador"ANSI_COLOR_RESET);
	}

	printf(ANSI_COLOR_BOLDWHITE"ENVIO HEADER %d AL COORDINADOR\n"ANSI_COLOR_RESET, header->codigoOperacion);

	free(header);
}

void enviarHeaderOperacionOK() {
	header_t* header = malloc(sizeof(header_t));

	header->codigoOperacion = 9;	// AVISO QUE ESTA SALIO OK LA OPERACION
	header->tamanioClave = -1; 		// TAMANIO ABSURDO

	if (send(socketCoordinador, header, sizeof(header_t), 0) < 0) {
		_exit_with_error(socketCoordinador, ANSI_COLOR_BOLDRED"No se pudo enviar el header de operacion OK al Coordinador"ANSI_COLOR_RESET);
	}

//	printf(ANSI_COLOR_BOLDWHITE"ENVIO HEADER %d AL COORDINADOR\n"ANSI_COLOR_RESET, header->codigoOperacion);

	free(header);
}

void enviarHeaderOperacionSETFail() {
	header_t* header = malloc(sizeof(header_t));

	header->codigoOperacion = 10;	// AVISO QUE ESTA SALIO OK LA OPERACION
	header->tamanioClave = -1; 		// TAMANIO ABSURDO

	if (send(socketCoordinador, header, sizeof(header_t), 0) < 0) {
		_exit_with_error(socketCoordinador, ANSI_COLOR_BOLDRED"No se pudo enviar el header de operacion OK al Coordinador"ANSI_COLOR_RESET);
	}

//	printf(ANSI_COLOR_BOLDWHITE"ENVIO HEADER %d AL COORDINADOR\n"ANSI_COLOR_RESET, header->codigoOperacion);

	free(header);
}

void enviarTamanioValor(int socket, int* tamanioValor){
	if (send(socket, tamanioValor, sizeof(int32_t), 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el tamanio del valor"ANSI_COLOR_RESET);
	}
}

void enviarValor(int socket, int tamanioValor, char* valor){
	if (send(socket, valor, tamanioValor, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el valor"ANSI_COLOR_RESET);
	}
}

int entradasLibres() {
	int contador = 0;

	for (int i=0; i<CANTIDADENTRADAS; i++) {
		storage = buscarEnStorage(i);
		if (storage[0] == '\0') {
			contador++;
		}
	}

	return contador;
}

void set(char* clave, char* valor){
	entrada_t* entrada;
	char* valorViejo;
	int posicion = -2; 			// LA INICIALIZO NEGATIVA PARA NOTAR ERROR EN LOS LOGS

	list_iterate(tablaEntradas, (void*) sumarUnoALasNoReferencias);		// LE SUMO UNO A LA CANTIDAD DE NO REFERENCIAS A TODAS LAS ENTRADAS (LA QUE HIZO EL SET, TRES LINEAS ABAJO SE SETEA EN 0)

	int espaciosNecesarios = ceil((double) (strlen(valor)) / (double) TAMANIOENTRADA);		// DEVUELVE REDONDEADO PARA ARRIBA

	log_info(logger, ANSI_COLOR_BOLDWHITE"ESPACIOS NECESARIOS PARA EL VALOR: %d"ANSI_COLOR_RESET, espaciosNecesarios);

	if ((entrada = buscarEnTablaDeEntradas(clave)) != NULL) {		// YA EXISTE UNA ENTRADA CON LA MISMA CLAVE, ENTONCES DEBO MODIFICAR EL STORAGE
		valorViejo = buscarEnStorage(entrada->numero);
		entrada->cantidadDeNoReferencias = 0;

		int espaciosNecesariosValorViejo = ceil((double) (strlen(valorViejo)) / (double) TAMANIOENTRADA);

		//  HAY QUE VER SI EL NUEVO VALOR OCUPA MENOS O MAS QUE EL NUEVO
		if (espaciosNecesarios > espaciosNecesariosValorViejo) {		// SI OCUPA MAS QUE EL ESPACIO VIEJO...
			limpiarValores(entrada);		// PRIMERO LIMPIO LOS VALORES QUE TENIA ASIGNADOS EN EL STORAGE

			if ( (posicion = hayEspaciosContiguosPara(espaciosNecesarios)) >= 0) {		// SI HAY ESPACIOS CONTIGUOS...
				asignarAEntrada(entrada, valor, espaciosNecesarios);
				entrada->numero = posicion;

				copiarValorAlStorage(entrada, valor, posicion);		// BAJO AL STORAGE EL VALOR

				if (!LEVANTODEDISCO) {				// SI ES QUE NO HAGO EL SET POR LEVANTAR DE DISCO...
					enviarHeaderOperacionOK();			// LE AVISO AL COORDINADOR QUE SALIO BIEN
				}
			} else if(hayEspaciosNoContiguosPara(espaciosNecesarios)) {
				compactar();

				posicion = hayEspaciosContiguosPara(espaciosNecesarios);
				asignarAEntrada(entrada, valor, espaciosNecesarios);
				entrada->numero = posicion;

				copiarValorAlStorage(entrada, valor, posicion);		// BAJO AL STORAGE EL VALOR
			} else {
				// UTILIZAR ALGORITMO DE REEMPLAZO
				if ((posicion = reemplazarSegunAlgoritmo(espaciosNecesarios)) >= 0) {
					asignarAEntrada(entrada, valor, espaciosNecesarios);
					entrada->numero = posicion;

					copiarValorAlStorage(entrada, valor, posicion);		// BAJO AL STORAGE EL VALOR
				}
			}

		}

		else if (espaciosNecesarios == espaciosNecesariosValorViejo) {	// SI NECESITO LOS QUE YA ESTAN ASIGNADOS
			limpiarValores(entrada);

			asignarAEntrada(entrada, valor, espaciosNecesarios);
			copiarValorAlStorage(entrada, valor, entrada->numero);

			if (!LEVANTODEDISCO) {				// SI ES QUE NO HAGO EL SET POR LEVANTAR DE DISCO...
				enviarHeaderOperacionOK();			// LE AVISO AL COORDINADOR QUE SALIO BIEN
			}
		}

		else {			// SI OCUPA MENOS ESPACIO QUE EL VIEJO VALOR
			limpiarValores(entrada);

			asignarAEntrada(entrada, valor, espaciosNecesarios);
			copiarValorAlStorage(entrada, valor, entrada->numero);

			if (!LEVANTODEDISCO) {				// SI ES QUE NO HAGO EL SET POR LEVANTAR DE DISCO...
				enviarHeaderOperacionOK();			// LE AVISO AL COORDINADOR QUE SALIO BIEN
			}
		}

		log_info(logger, ANSI_COLOR_BOLDCYAN"Se modifico el valor de la clave %s con %s"ANSI_COLOR_RESET, entrada->clave, valor);

	}
	else {		// SINO CREO UNA NUEVA ENTRADA

		entrada = malloc(sizeof(entrada_t));

		/* Creo entrada */
		asignarAEntrada(entrada, valor, espaciosNecesarios);
		entrada->clave = malloc(strlen(clave) + 1);
		strcpy(entrada->clave, clave);
		entrada->cantidadDeNoReferencias = 0;

		/*************** FINALIZO LO DE LA ENTRADA **************************/

		if ((posicion = hayEspaciosContiguosPara(espaciosNecesarios)) >= 0) {		// SI LA POSICION ES >= 0 OK (SINO DEVUELVE -1)
			asignarAEntrada(entrada, valor, espaciosNecesarios);		// SETEO LOS DATOS DE LA ENTRADA
			entrada->numero = posicion;

			copiarValorAlStorage(entrada, valor, posicion);

			if (!LEVANTODEDISCO) {				// SI ES QUE NO HAGO EL SET POR LEVANTAR DE DISCO...
				enviarHeaderOperacionOK();			// LE AVISO AL COORDINADOR QUE SALIO BIEN
			}
		}  else if(hayEspaciosNoContiguosPara(espaciosNecesarios)) {
			compactar();

			posicion = hayEspaciosContiguosPara(espaciosNecesarios);
			asignarAEntrada(entrada, valor, espaciosNecesarios);
			entrada->numero = posicion;

			copiarValorAlStorage(entrada, valor, posicion);		// BAJO AL STORAGE EL VALOR
		} else {
			// REEMPLAZAR SEGUN ALGORITMO
			if ((posicion = reemplazarSegunAlgoritmo(espaciosNecesarios)) >= 0) {
				asignarAEntrada(entrada, valor, espaciosNecesarios);
				entrada->numero = posicion;

				copiarValorAlStorage(entrada, valor, posicion);
			}
		}

		if (posicion == -1) {
			log_error(logger, ANSI_COLOR_BOLDRED"No se pudo realizar el SET por no haber suficientes entradas atomicas"ANSI_COLOR_RESET);
			enviarHeaderOperacionSETFail();
		} else {
			list_add(tablaEntradas, entrada);
			log_info(logger, ANSI_COLOR_BOLDCYAN"Se agrego la entrada: Clave %s - Entrada %d - Tamanio Valor %d "ANSI_COLOR_RESET, entrada->clave, entrada->numero, entrada->tamanio_valor);
			log_info(logger, ANSI_COLOR_BOLDCYAN"Se agrego el valor %s al storage en la posicion %d"ANSI_COLOR_RESET, valor, posicion);
		}
	}
}

int reemplazarSegunAlgoritmo(int espaciosNecesarios) {
	// ESTO DEBERIA LIBERAR LOS ESPACIOS ATOMICOS CORRESPONDIENTES, COMPACTAR, VERIFICAR SI YA ENTRA Y DEVOLVER EL ESPACIO DONDE IRIA --> LOOP
	int posicion = -1;
	int posicionEntrada;
	entrada_t* entradaSeleccionada;

	if ((cantidadValoresAtomicos() + entradasLibres()) < espaciosNecesarios) {
		return posicion;
	}

	do {
		if (strcmp(ALGORITMOREEMPLAZO, "CIRC") == 0) {
			if (DEBUG) {
				printf(ANSI_COLOR_BOLDMAGENTA"STORAGEAPUNTADO: %d\n"ANSI_COLOR_RESET, STORAGEAPUNTADO);
			}

			entradaSeleccionada = buscarEntrada(STORAGEAPUNTADO, &posicionEntrada);

			if (entradaSeleccionada != NULL) {
				if (entradaSeleccionada->largo == 1) {		// SI ES ATOMICA LA ENTRADA...
					limpiarValores(entradaSeleccionada);

					printf(ANSI_COLOR_BOLDMAGENTA"VALOR DE ENTRADA %d BORRADO: %s\n"ANSI_COLOR_RESET, entradaSeleccionada->numero, storage);

//					store(entradaSeleccionada->clave);

					free(entradaSeleccionada->clave);			// LIBERO LA CLAVE DE LA ENTRADA

					list_remove(tablaEntradas, posicionEntrada);		// Y LA SACO DE LA LISTA

					free(entradaSeleccionada);
				}
			}

			STORAGEAPUNTADO++;		// PASO A LA OTRA ENTRADA

			if (STORAGEAPUNTADO == CANTIDADENTRADAS) {		// SI ESTA EN LA ULT
				STORAGEAPUNTADO = 0;			// SE VUELVE AL PRINCIPIO
			}
		}

		if (strcmp(ALGORITMOREEMPLAZO, "BSU") == 0) {
			entradaSeleccionada = buscarEntradaAtomicaMasGrande(&posicionEntrada);

			limpiarValores(entradaSeleccionada);

//			store(entradaSeleccionada->clave);	// NO SE SI HAY QUE HACERLO

			free(entradaSeleccionada->clave);			// LIBERO LA CLAVE DE LA ENTRADA

			list_remove(tablaEntradas, posicionEntrada);		// Y LA SACO DE LA LISTA

			free(entradaSeleccionada);
		}

		if (strcmp(ALGORITMOREEMPLAZO, "LRU") == 0) {
			entradaSeleccionada = buscarEntradaMenosReferenciada(&posicionEntrada);

			limpiarValores(entradaSeleccionada);

//			store(entradaSeleccionada->clave);	// NO SE SI HAY QUE HACERLO

			free(entradaSeleccionada->clave);			// LIBERO LA CLAVE DE LA ENTRADA

			list_remove(tablaEntradas, posicionEntrada);		// Y LA SACO DE LA LISTA

			free(entradaSeleccionada);
		}

	} while(hayEspaciosNoContiguosPara(espaciosNecesarios) == 0);			// MIENTRAS QUE NO HAYA ESPACIOS (NO) CONTIGUOS, QUE SIGA LIBERANDO

	if ((posicion = hayEspaciosContiguosPara(espaciosNecesarios)) >= 0) {	// SI YA ESTABAN CONTIGUOS, ENTONCES DEVUELVO
		if (!LEVANTODEDISCO) {				// SI ES QUE NO HAGO EL SET POR LEVANTAR DE DISCO...
			enviarHeaderOperacionOK();			// LE AVISO AL COORDINADOR QUE SALIO BIEN
		}
		return posicion;
	} else {																// SINO, PRIMERO COMPACTO, ASIGNO Y DEVUELVO
		compactar();
		posicion = hayEspaciosContiguosPara(espaciosNecesarios);
	}

	return posicion;
}

int cantidadValoresAtomicos() {
	int contador = 0;
	entrada_t* entrada;

	for (int i=0; i<list_size(tablaEntradas); i++) {
		entrada = list_get(tablaEntradas, i);

		if (entrada->largo == 1) {
			contador++;
		}
	}

	return contador;
}

entrada_t* buscarEntradaAtomicaMasGrande(int* posicion) {
	entrada_t* entrada;
	entrada_t* entradaMasGrande;
	int maximo = -1;


	ordenarTablaDeEntradas();
	for (int i=0; i<list_size(tablaEntradas); i++) {
		entrada = list_get(tablaEntradas, i);

		if (entrada->largo == 1) {
			if (entrada->tamanio_valor > maximo) {
				entradaMasGrande = entrada;
				maximo = entrada->tamanio_valor;
				*posicion = i;
			}
		}
	}

	return entradaMasGrande;
}

entrada_t* buscarEntradaMenosReferenciada(int* posicion) {
	entrada_t* entrada;
	entrada_t* entradaMenosReferenciada;
	int maximo = -1;

	ordenarTablaDeEntradas();
	for (int i=0; i<list_size(tablaEntradas); i++) {
		entrada = list_get(tablaEntradas, i);

		if (entrada->largo == 1) {				// TIENE QUE SER ATOMICA
			if (entrada->cantidadDeNoReferencias > maximo) {	// SI HACE MAS QUE NO SE REFERENCIO QUE EL MAXIMO...
				entradaMenosReferenciada = entrada;				// ASIGNO
				maximo = entrada->cantidadDeNoReferencias;
				*posicion = i;
			}
		}
	}

	return entradaMenosReferenciada;
}

void sumarUnoALasNoReferencias(entrada_t* entrada) {
	entrada->cantidadDeNoReferencias += 1;
}

int hayEspaciosNoContiguosPara(int espaciosNecesarios) {
	int contador = 0;

	for (int i=0; i<CANTIDADENTRADAS; i++) {
		storage = buscarEnStorage(i);

		if(storage[0] == '\0') {
			contador++;
		}

		if (contador == espaciosNecesarios) {
			return 1;
		}
	}

	return 0;
}

int hayEspaciosContiguosPara(int espaciosNecesarios) {
	int contador = 0;

	for (int i=0; i<CANTIDADENTRADAS; i++) {
		storage = buscarEnStorage(i);
		if (storage[0] == '\0') {		// SI ESA POSICION ESTA VACIO
			contador++;
		} else {
			contador = 0;
		}

		if (contador == espaciosNecesarios) {
			return i+1-espaciosNecesarios;			// DEVUELVO LA POSICION INICIAL DONDE SE TENDRIA QUE ALMACENAR EL VALOR
		}
	}

	return -1;
}

void asignarAEntrada(entrada_t* entrada, char* valor, int largo) {
	entrada->tamanio_valor = strlen(valor) + 1;
	entrada->largo = largo;
}

void copiarValorAlStorage(entrada_t* entrada, char* valor, int posicion) {
	storage = buscarEnStorage(posicion);
	memcpy(storage, valor, strlen(valor));

	log_info(logger, ANSI_COLOR_BOLDWHITE"Se copio el valor %s al storage %d", valor, posicion);
}

void limpiarValores(entrada_t* entrada) {
	for (int i=0; i<entrada->largo; i++) {
		storage = buscarEnStorage(entrada->numero+i);
		storage[0] = '\0';
	}
}

void store(char* clave){
	entrada_t* entrada;
	int fd;
	char* mem_ptr;
	char* valor = string_new();
	char* directorioMontaje = malloc(strlen(PUNTOMONTAJE) + strlen(clave) + 1);

	mkdir(PUNTOMONTAJE, 0777);			// CREO EL DIRECTORIO SI NO ESTA CREADO

	strcpy(directorioMontaje, PUNTOMONTAJE);

	strcat(directorioMontaje, clave);		// DIRECTORIO DE MONTAJE TIENE LA DIRECCION COMPLETA DONDE SE VA A GUARDAR EL VALOR

	entrada = buscarEnTablaDeEntradas(clave);	// BUSCO LA ENTRADA POR LA CLAVE

	if (entrada != NULL) {		// PUEDE HABER SIDO REEMPLAZADA
		if (!DUMP) {		// SI ES QUE NO ESTA HACIENDO STORE POR EL DUMP...
			list_iterate(tablaEntradas, (void*) sumarUnoALasNoReferencias);		// LE SUMO UNO A LA CANTIDAD DE NO REFERENCIAS A TODAS LAS ENTRADAS (LA QUE HIZO EL SET ABAJO SE SETEA EN 0)
			entrada->cantidadDeNoReferencias = 0;
		}
		for (int i=0; i<entrada->largo; i++) {			// ENTRADA LARGO ES EL NUM DE ESPACIOS QUE OCUPA EL VALOR
			storage = buscarEnStorage(entrada->numero + i);
			string_append(&valor, storage); 		// CONCATENO EL STRING USANDO TODOS LOS ESPACIOS DEL STORAGE
		}

		if((fd = open(directorioMontaje, O_CREAT | O_RDWR , S_IRWXU )) < 0){
			log_error(logger, ANSI_COLOR_BOLDRED"No se pudo realizar el open por path inexistente"ANSI_COLOR_RESET);
		}

		size_t tamanio = entrada->tamanio_valor;

		lseek(fd, tamanio-1, SEEK_SET);
		write(fd, "", 1);

		mem_ptr = mmap(NULL, tamanio , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);

		memcpy(mem_ptr, valor, tamanio-1);

		msync(mem_ptr, tamanio-1, MS_SYNC);

		munmap(mem_ptr, tamanio);

		free(directorioMontaje);
		free(valor);

		if (!DUMP) {
			enviarHeaderOperacionOK();		// AVISO QUE SALIO BIEN
		}
	}

	else {			// SI LA ENTRADA NO EXISTE POR QUE SE REEMPLAZO
		header_t* header = malloc(sizeof(header_t));

		header->codigoOperacion = 6;		// ERROR STORE CLAVE REEMPLAZADA
		header->tamanioClave = -1;			// TAMANIO ABSURDO

		log_error(logger, ANSI_COLOR_BOLDRED"ERROR STORE clave %s previamente reemplazada"ANSI_COLOR_RESET, clave);

		if (send(socketCoordinador, header, sizeof(header_t), 0) < 0) {
			_exit_with_error(socketCoordinador, ANSI_COLOR_BOLDRED"No se pudo enviar el header de ERROR de STORE"ANSI_COLOR_RESET);
		}

		free(header);
	}
}

/* Busca la entrada que contenga esa clave */
entrada_t* buscarEnTablaDeEntradas(char* clave) {
	entrada_t* entrada;

	for(int i=0; i<list_size(tablaEntradas); i++) {	// RECORRO LA LISTA DE ENTRADAS
		sem_wait(&mutexTablaDeEntradas);
		entrada = list_get(tablaEntradas, i);		// VOY TOMANDO ELEMENTOS
		sem_post(&mutexTablaDeEntradas);

		if (strcmp(entrada->clave, clave) == 0) {	// SI LA CLAVE DE LA ENTRADA COINCIDE CON LA DEL STORE
			return entrada;							// DEVUELVO LA ENTRADA ENCONTRADA
		}
	}

	return NULL;
}

/*Busca el data que contenga esa entrada*/
char* buscarEnStorage(int numeroEntrada) {
	return storageFijo + numeroEntrada * TAMANIOENTRADA;
}

entrada_t* buscarEntrada(int numeroDeStorage, int* posicion) {
	entrada_t* entrada;

	for (int i=0; i<list_size(tablaEntradas); i++) {
		entrada = list_get(tablaEntradas, i);

		if (numeroDeStorage == entrada->numero) {	// COMO ES ATOMICA, TIENE QUE SER LA MISMA
			*posicion = i;
			return entrada;
		}
	}

	return NULL;
}

/* Recibir sentencia del coordinador*/
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

void dump() {
	entrada_t* entrada;
	while (1) {
		sleep(INTERVALODUMP);

		mostrarTablaDeEntradas();

		DUMP = 1;
		sem_wait(&mutexOperaciones);
		for (int i=0; i<list_size(tablaEntradas); i++) {
			sem_wait(&mutexTablaDeEntradas);
			entrada = list_get(tablaEntradas, i);
			sem_post(&mutexTablaDeEntradas);

			store(entrada->clave);
		}
		sem_post(&mutexOperaciones);
		DUMP = 0;

		log_info(logger, ANSI_COLOR_BOLDYELLOW"*********** TERMINO EL DUMP *************"ANSI_COLOR_RESET);
	}
}

void mostrarTablaDeEntradas() {
	entrada_t* entrada;

	sem_wait(&mutexTablaDeEntradas);
	for (int i=0; i<list_size(tablaEntradas); i++) {
		entrada = list_get(tablaEntradas, i);

		printf(ANSI_COLOR_BOLDWHITE"Num. Entrada: %d - Clave: %s - Largo: %d\n"ANSI_COLOR_RESET, entrada->numero, entrada->clave, entrada->largo);
	}
	sem_post(&mutexTablaDeEntradas);
}

void mostrarStorage(){
	for (int i=0; i<CANTIDADENTRADAS; i++) {
			storage = buscarEnStorage(i);
			log_info(logger, "Storage %d: %.*s", i, TAMANIOENTRADA, storage);
	}
}

void compactar(){
	int j = 0;
	char* storageCompactadoFijo = malloc(CANTIDADENTRADAS * TAMANIOENTRADA);
	char* storageCompactado;
	entrada_t* entrada;

	for (int i=0; i<CANTIDADENTRADAS; i++) {
		storageCompactado = storageCompactadoFijo + i * TAMANIOENTRADA;
		strcpy(storageCompactado, "");
	}

	ordenarTablaDeEntradas();

	for (int i=0; i<list_size(tablaEntradas); i++) {
		entrada = list_get(tablaEntradas, i);

		if (entrada->numero != j) {				// SI LA ENTRADA ESTA EN OTRA POSICION DE J (J VA RECORRIENDO EN ORDEN DE VACIOS), ENTONCES COPIO
			storageCompactado = storageCompactadoFijo + j * TAMANIOENTRADA;
			storage = buscarEnStorage(entrada->numero);
			strncpy(storageCompactado, storage, entrada->tamanio_valor-1);		// COPIO TODO EL VALOR (MENOS EL \0)

			if (DEBUG) {
				printf(ANSI_COLOR_BOLDWHITE"Movi ENTRADA %d a %d - Valor %s\n"ANSI_COLOR_RESET, entrada->numero, j, storageCompactado);
			}

			entrada->numero = j;
		} else {						// SI ES LA MISMA, LA COPIO DE NUEVO AL NUEVO STORAGE
			storageCompactado = storageCompactadoFijo + j * TAMANIOENTRADA;
			storage = buscarEnStorage(entrada->numero);
			strncpy(storageCompactado, storage, entrada->tamanio_valor-1);		// COPIO TODO EL VALOR (MENOS EL \0)
		}

		j += entrada->largo;		// MUEVO J
	}

	free(storageFijo);
	storageFijo = storageCompactadoFijo;

	if (!LEVANTODEDISCO && !COMPACTARPORPEDIDO) {				// SI ES QUE NO HAGO EL COMPACTAR POR LEVANTAR DE DISCO NI POR PEDIDO...
		enviarHeader(socketCoordinador, 7, -1);
	}

	log_info(logger, ANSI_COLOR_BOLDYELLOW"La compactacion termino"ANSI_COLOR_RESET);
}

int comparadorNumeroEntrada(entrada_t* entrada, entrada_t* entrada2) {
	return entrada->numero < entrada2->numero;
}

void ordenarTablaDeEntradas() {
	sem_wait(&mutexTablaDeEntradas);
	list_sort(tablaEntradas, (void*) comparadorNumeroEntrada);
	sem_post(&mutexTablaDeEntradas);
}


int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = pipeHandler;
	sigaction(SIGPIPE, &finalizacion, NULL);

	configurarLogger();
	crearConfig();
	setearConfigEnVariables();
	socketCoordinador = conectarSocket();

	/************************************** CREO THREAD PARA DUMP **********************************/

	pthread_t threadDump;

	if(pthread_create(&threadDump, NULL, (void *)dump, NULL) != 0) {
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo crear el hilo de dump"ANSI_COLOR_RESET);
		exit(-1);
	}else{
		log_info(logger, ANSI_COLOR_BOLDCYAN"Se creo el hilo de dump"ANSI_COLOR_RESET);
	}

	tablaEntradas = list_create();
	listaStorage = list_create();

	reciboHandshake(socketCoordinador);
	envioIdentificador(socketCoordinador);

	conectarConCoordinador(socketCoordinador);

	close(socketCoordinador);

	return EXIT_SUCCESS;
}
