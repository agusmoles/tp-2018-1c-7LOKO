/******** CLIENTE INSTANCIAS *********/

#include "instancia.h"
#include <sys/mman.h>
#include <fcntl.h>

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
	TAMANIOENTRADA = config_get_int_value(config, "Tamanio de Entrada");
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
			recibirInstruccion(socket);
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

	if (send(socket, identificador, strlen(identificador)+1, 0) < 0) {
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el identificador"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio correctamente el identificador"ANSI_COLOR_RESET);
}

void pipeHandler() {
	printf(ANSI_COLOR_BOLDRED"***********************************EL SERVIDOR SE CERRO***********************************\n"ANSI_COLOR_RESET);
	exit(1);
}

/* Recibir operacion de Coordinador */
void recibirInstruccion(int socket){

	header_t* buffer_header = malloc(sizeof(header_t));
	char* bufferClave;
	char* bufferValor;
	int32_t* tamanioValor;

	if(recv(socket, buffer_header, sizeof(header_t), MSG_WAITALL)< 0){
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No se pudo recibir el Header"ANSI_COLOR_RESET);
	}

	bufferClave = malloc(buffer_header->tamanioClave);
	tamanioValor = malloc(sizeof(int32_t));

	switch(buffer_header->codigoOperacion){
	case 1: /* SET */
		/* Recibo clave y valor */
		recibirClave(socket, buffer_header, bufferClave);
		recibirTamanioValor(socket, tamanioValor);
		bufferValor = malloc(*tamanioValor);
		recibirValor(socket, tamanioValor, bufferValor);

		set(bufferClave, bufferValor);

		break;
	case 2: /* STORE */
		/* Recibo clave */
		recibirClave(socket, buffer_header, bufferClave);

		store(bufferClave);
		break;
	default:
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No existe el codigo de operacion"ANSI_COLOR_RESET);
		break;
	}
//
	free(buffer_header);
	free(tamanioValor);

	/* ESTOS NO
    free(bufferClave);
	free(bufferValor);
*/
}

void set(char* clave, char* valor){
	entrada_t* entrada;
	data_t* data;
	int cantidadEntradas = list_size(tablaEntradas);
	int entradasNecesarias;

	if ((entrada = buscarEnTablaDeEntradas(clave)) != NULL) {		// YA EXISTE UNA ENTRADA CON LA MISMA CLAVE, ENTONCES DEBO MODIFICAR EL STORAGE
		data = buscarEnStorage(entrada->numero);

		free(data->valor);		// YA NO ME INTERESA EL VIEJO VALOR, LIBERO MEMORIA

		data->valor = malloc(strlen(valor) + 1);	// SETEO MEMORIA

		strcpy(data->valor, valor);			// COPIO EL NUEVO VALOR A LA DATA

		log_info(logger, ANSI_COLOR_BOLDGREEN"Se modifico el valor de la clave %s con %s"ANSI_COLOR_RESET, entrada->clave, data->valor);

	}
	else {		// SINO CREO UNA NUEVA ENTRADA

		entrada = malloc(sizeof(entrada_t));
		data = malloc(sizeof(data_t));

		/* Creo entrada */
		entrada->tamanio_valor = strlen(valor) + 1;
		entrada->clave = malloc(strlen(clave) + 1);
		strcpy(entrada->clave, clave);


		if(strlen(valor) <= TAMANIOENTRADA){
			entradasNecesarias = 1;
		}else{
			entradasNecesarias = 2;
		}

	/* ESTE if ES PARA CONTEMPLAR CUANDO SE NECESITAN 2 ENTRADAS */
	if(entradasNecesarias > 1){

			entrada->tamanio_valor = TAMANIOENTRADA;

			entrada_t* entrada2;							/* CREO OTRA ENTRADA */
			entrada2 = malloc(sizeof(entrada_t));

			entrada2->clave = malloc(strlen(clave)+1);
			strcpy(entrada2->clave, entrada->clave);

			entrada2->tamanio_valor = strlen(valor) - TAMANIOENTRADA;
			entrada->numero = list_size(tablaEntradas);
			entrada2->numero = list_size(tablaEntradas) + 1;

			if(CANTIDADENTRADAS - cantidadEntradas < entradasNecesarias){
				reemplazarSegun(ALGORITMOREEMPLAZO, entrada);
				reemplazarSegun(ALGORITMOREEMPLAZO, entrada2);
			}else{
				list_add(tablaEntradas, entrada);
				list_add(tablaEntradas, entrada2);
			}

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se agregaron las entradas: Clave %s - Entrada %d - Tamanio Valor %d \n Clave %s - Entrada %d - Tamanio Valor %d "ANSI_COLOR_RESET, entrada->clave, entrada->numero, entrada->tamanio_valor,entrada2->clave,entrada2->numero,entrada2->tamanio_valor);;

			char* valor1 = malloc(entrada->tamanio_valor);
			char* valor2 = malloc(entrada2->tamanio_valor);

			valor1 = string_substring(valor, 0, TAMANIOENTRADA);									// ACA SEPARO EL VALOR
			valor2 = string_substring(valor, TAMANIOENTRADA, sizeof(valor) + 1 - TAMANIOENTRADA);		// EN DOS STRINGS

			data->numeroEntrada = entrada->numero;
			data->valor = malloc(entrada->tamanio_valor);
			strcpy(data->valor, valor1);

			data_t* data2;							/* CREO OTRO DATA */
			data2 = malloc(sizeof(data_t));

			data2->numeroEntrada = entrada2->numero;
			data2->valor = malloc(entrada2->tamanio_valor);
			strcpy(data2->valor, valor2);

			list_add(listaStorage, data);
			list_add(listaStorage, data2);

			free(valor1);
			free(valor2);
			log_info(logger, ANSI_COLOR_BOLDGREEN"Se agrego informacion: Entrada %d - Valor %s al Storage \n Entrada %d - Valor %s al Storage"ANSI_COLOR_RESET, data->numeroEntrada, data->valor, data2->numeroEntrada, data2->valor);

		}else{

			entrada->numero = list_size(tablaEntradas);

			list_add(tablaEntradas, entrada);
			cantidadEntradas ++;

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se agrego la entrada: Clave %s - Entrada %d - Tamanio Valor %d "ANSI_COLOR_RESET, entrada->clave, entrada->numero, entrada->tamanio_valor);

			data->numeroEntrada = entrada->numero;
			data->valor = malloc(entrada->tamanio_valor);
			strcpy(data->valor, valor);

			list_add(listaStorage, data);

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se agrego informacion: Entrada %d - Valor %s al Storage"ANSI_COLOR_RESET, data->numeroEntrada, data->valor);
		}
	}
}

void store(char* clave){
	entrada_t* entrada;
	data_t* storage;
	int fd;
	char* mem_ptr;
	char* directorioMontaje = malloc(strlen(PUNTOMONTAJE) + strlen(clave) + 1);

	mkdir(PUNTOMONTAJE, 0777);

	strcpy(directorioMontaje, PUNTOMONTAJE);

	strcat(directorioMontaje, clave);

	claveBuscada = malloc(strlen(clave)+ 1);
	strcpy(claveBuscada, clave);

	entrada = buscarEnTablaDeEntradas(clave);	// BUSCO LA ENTRADA POR LA CLAVE
	storage = buscarEnStorage(entrada->numero);	// BUSCO EL STORAGE DE ESE NUM DE ENTRADA

	if((fd = open(directorioMontaje, O_CREAT | O_RDWR , S_IRWXU )) < 0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo realizar el open "ANSI_COLOR_RESET);
	}

	size_t tamanio = strlen(storage->valor) + 1;

	lseek(fd, tamanio-1, SEEK_SET);
	write(fd, "", 1);

	mem_ptr = mmap(NULL, tamanio , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);

	memcpy(mem_ptr, storage->valor, tamanio);

	msync(mem_ptr, tamanio, MS_SYNC);

	munmap(mem_ptr, tamanio);

	free(directorioMontaje);
}

/* Busca la entrada que contenga esa clave */
entrada_t* buscarEnTablaDeEntradas(char* clave) {
	entrada_t* entrada;

	for(int i=0; i<list_size(tablaEntradas); i++) {	// RECORRO LA LISTA DE ENTRADAS
		entrada = list_get(tablaEntradas, i);		// VOY TOMANDO ELEMENTOS

		if (strcmp(entrada->clave, clave) == 0) {	// SI LA CLAVE DE LA ENTRADA COINCIDE CON LA DEL STORE
			return entrada;							// DEVUELVO LA ENTRADA ENCONTRADA
		}
	}

	return NULL;
}

/*Busca el data que contenga esa entrada*/
data_t* buscarEnStorage(int entrada) {
	data_t* storage;

	for(int i=0; i<list_size(listaStorage); i++) {	// RECORRO LA LISTA DE STORAGE
		storage = list_get(listaStorage, i);		// VOY TOMANDO ELEMENTOS

		if (storage->numeroEntrada == entrada) {	// SI LA ENTRADA COINCIDE CON LA DE LA LISTA DEL STORAGE
			return storage;							// DEVUELVO EL STORAGE ENCONTRADO
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


int main(void) {
	struct sigaction finalizacion;
	finalizacion.sa_handler = pipeHandler;
	sigaction(SIGPIPE, &finalizacion, NULL);

	configurarLogger();
	crearConfig();
	setearConfigEnVariables();
	int socket = conectarSocket();

	tablaEntradas = list_create();
	listaStorage = list_create();

	reciboHandshake(socket);
	envioIdentificador(socket);


	conectarConCoordinador(socket);



	close(socket);

	return EXIT_SUCCESS;
}
