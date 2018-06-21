/******** CLIENTE INSTANCIAS *********/

#include "AlgoritmosReemplazo.h"
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
	CANTIDADENTRADAS = config_get_int_value(config, "Cantidad de Entradas");

    storage = calloc(CANTIDADENTRADAS, sizeof(char*)); 	// CREO VECTOR CON TAMAÑO PARA PUNTERO DE CHARS

	for (int i=0; i<CANTIDADENTRADAS; i++) {
		storage[i] = malloc(TAMANIOENTRADA);
		strcpy(storage[i], "");  						// INICIALIZO CON STRING VACIO
    }

	tablaEntradas = list_create();
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
	entrada_t* entrada;
	char* data;
	int* tamanioValorStatus = malloc(sizeof(int));
	char* valorStatus;

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
	case 3: /* Status */
		recibirClave(socket, buffer_header, bufferClave);

		if((entrada = buscarEnTablaDeEntradas(bufferClave)) != NULL && (data = buscarEnStorage(entrada->numero)) != NULL){
				*tamanioValorStatus = entrada->tamanio_valor;
				valorStatus = malloc(*tamanioValorStatus);
				strcpy(valorStatus, data);

				enviarTamanioValor(socket, tamanioValorStatus);
				log_info(logger, "Se envio el tamanio del valor al coordinador");
				enviarValor(socket, *tamanioValorStatus, valorStatus);
				log_info(logger, "Se envio el valor al coordinador");
				free(valorStatus);
			}else{
				*tamanioValorStatus = -1;
				enviarTamanioValor(socket, tamanioValorStatus);
				log_info(logger, "Se envio el tamanio del valor al coordinador");
		}

		break;
	default:
		_exit_with_error(socket, ANSI_COLOR_BOLDRED"No existe el codigo de operacion"ANSI_COLOR_RESET);
		break;
	}
//
	free(buffer_header);
	free(tamanioValor);
	free(tamanioValorStatus);
	/* ESTOS NO
    free(bufferClave);
	free(bufferValor);
*/
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

void set(char* clave, char* valor){
	entrada_t* entrada;
	t_list* entradasPendientes;
	t_list* datasPendientes;

	int entradasNecesarias = ceil(strlen(valor)/TAMANIOENTRADA); // ceil() redondea siempre para arriba.

	log_info(logger, ANSI_COLOR_BOLDWHITE"Entradas necesarias: %d"ANSI_COLOR_RESET, entradasNecesarias);

	// FALTA VER CASOS QUE EL VALOR NUEVO SEA MAS CHICO O MAS GRANDE QUE EL VALOR VIEJO --> LIBERAR LOS OTROS O PEDIR MAS ESPACIO

	if ((entrada = buscarEnTablaDeEntradas(clave)) != NULL) {		// YA EXISTE UNA ENTRADA CON LA MISMA CLAVE, ENTONCES DEBO MODIFICAR EL STORAGE
		char* data = buscarEnStorage(entrada->numero);

		strcpy(data, valor);			// COPIO EL NUEVO VALOR A LA DATA

		log_info(logger, ANSI_COLOR_BOLDGREEN"Se modifico el valor de la clave %s con %s"ANSI_COLOR_RESET, entrada->clave, data);

	}
	else {		// ACA SE CREAN ENTRADAS

		datasPendientes = list_create();
		entradasPendientes = list_create();

		for(int i=0; i<entradasNecesarias; i++){
			entrada = malloc(sizeof(entrada_t));
			entrada->clave = malloc(strlen(clave) + 1);
			strcpy(entrada->clave, clave);

			char* data = malloc(TAMANIOENTRADA);
			char* valueModificada = string_substring(valor, i*TAMANIOENTRADA, TAMANIOENTRADA);

			entrada->tamanio_valor = strlen(valueModificada)+1;

			strcpy(data, valueModificada);

			list_add(datasPendientes, data);
			list_add(entradasPendientes, entrada);
			free(valueModificada);
		}

		agregarPendientes(entradasPendientes, datasPendientes);
		list_destroy(datasPendientes);
		list_destroy(entradasPendientes);
	}
}

void agregarPendientes(t_list* entradas, t_list* datas){
	entrada_t* entrada;
	char* data;
	int aux;

	while(list_size(entradas) > entradasContiguasDisponibles()){
		reemplazarSegun();
	}

	for(int k=0; k<list_size(datas); k++){
		aux = agregarAStorage(list_get(datas, k));
		entrada = list_get(entradas, k);
		entrada->numero = aux;
		list_add(tablaEntradas, entrada);
		data = list_get(datas,k);

		log_info(logger, ANSI_COLOR_BOLDGREEN"Se agrego la entrada: Clave %s - Entrada %d - Tamanio Valor %d "ANSI_COLOR_RESET, entrada->clave, entrada->numero, entrada->tamanio_valor);;
		log_info(logger, ANSI_COLOR_BOLDGREEN"Se agrego informacion: Entrada %d - Valor %s al Storage"ANSI_COLOR_RESET, aux, data);
	}
}

int agregarAStorage(char* data, int entradasNecesarias){ //falta aca que agregue los datas al array de manera que queden contiguas
	int contador = 0;								//y se agregue el bloque de datas en el primer lugar que se encuentre libre
	int resultado = 0;
	int posicion;

	for(int i=0; i<CANTIDADENTRADAS; i++){
		if(strcmp(storage[i],"") == 0){
			contador++;
		}else{
			if(contador > resultado){
			resultado = contador;
			}
			contador = 0;
		}
		if(resultado >= entradasNecesarias){
			posicion = i - entradasNecesarias;
		}
		return i;
	}


	return -1;
}

int entradasContiguasDisponibles(){
	int contador = 0;
	int resultado = 0;

	for(int i=0; i<CANTIDADENTRADAS; i++){
		if(strcmp(storage[i], "") == 0){
			contador++;
		}else{
			if(contador > resultado){
				resultado = contador;
			}
			contador = 0;
		}
	}
	return resultado;
}

void store(char* clave){
	entrada_t* entrada;
	char* data;
	int fd;
	char* mem_ptr;
	char* directorioMontaje = malloc(strlen(PUNTOMONTAJE) + strlen(clave) + 1);

	mkdir(PUNTOMONTAJE, 0777);

	strcpy(directorioMontaje, PUNTOMONTAJE);

	strcat(directorioMontaje, clave);

	claveBuscada = malloc(strlen(clave)+ 1);
	strcpy(claveBuscada, clave);

	entrada = buscarEnTablaDeEntradas(clave);	// BUSCO LA ENTRADA POR LA CLAVE
	data = buscarEnStorage(entrada->numero);	// BUSCO EL STORAGE DE ESE NUM DE ENTRADA

	if((fd = open(directorioMontaje, O_CREAT | O_RDWR , S_IRWXU )) < 0){
		log_error(logger, ANSI_COLOR_BOLDRED"No se pudo realizar el open "ANSI_COLOR_RESET);
	}

	size_t tamanio = strlen(data) + 1;

	lseek(fd, tamanio-1, SEEK_SET);
	write(fd, "", 1);

	mem_ptr = mmap(NULL, tamanio , PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);

	memcpy(mem_ptr, data, tamanio);

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
char* buscarEnStorage(int entrada) {
	char* data;
	data = storage[entrada];
	return data;
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

	reciboHandshake(socket);
	envioIdentificador(socket);


	conectarConCoordinador(socket);



	close(socket);

	return EXIT_SUCCESS;
}
