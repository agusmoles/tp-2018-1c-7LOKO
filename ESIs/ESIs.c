#include "ESIs.h"


int main(){
	configure_logger();
	crearConfig();
	setearConfigEnVariables();
	int socketPlanificador,socketCoordinador;
	FILE* script = fopen("ESI_MultiClave","r");
	char* instruccion;
	size_t len = 0;
	int sentencias = 0;
	char* buffer = malloc(strlen("OPOK") + 1);
	int caracteresLeidos;

	socketPlanificador = conect_to_server(IPPLANIFICADOR,PUERTOPLANIFICADOR);
	recibirHandshake(socketPlanificador,"******PLANIFICADOR HANDSHAKE******");
	socketCoordinador = conect_to_server(IPCOORDINADOR,PUERTOCOORDINADOR);
	recibirHandshake(socketCoordinador,"******COORDINADOR HANDSHAKE******");

	envioIdentificador(socketCoordinador);

	administrarID(socketPlanificador,socketCoordinador);

	sentencias = cantidadSentencias(script);	// ASIGNO LA CANTIDAD DE SENTENCIAS (LINEAS) DEL ESI

	fseek(script,0,SEEK_SET);			// MUEVO EL PUNTERO AL INICIO PORQUE ESTABA AL FINAL POR LA FUNCION CANTIDADSENTENCIAS()

	while((caracteresLeidos = getline(&instruccion,&len,script)) != -1){		// LEO LAS INSTURCCIONES
		sentencias--;

		log_info(logger, ANSI_COLOR_BOLDYELLOW"Esperando EXEOR..."ANSI_COLOR_RESET);

		recibirMensaje(socketPlanificador,"EXEOR");		// ESPERO ORDEN DE EJECUCION

		ejecutarInstruccion(instruccion,socketCoordinador, socketPlanificador);	// EJECUTO

		if (recv(socketCoordinador, buffer, strlen("OPOK") + 1, 0) < 0) {		// RECIBO RESULTADO DEL COORDINADOR
			exitError(socketCoordinador, "No se pudo recibir el resultado de la operacion");
		}

		if (strcmp(buffer, "OPBL") == 0) {		// SI ES QUE SE BLOQUEO EL ESI, VUELVO A EJECUTAR LA MISMA LINEA
			fseek(script, -caracteresLeidos, SEEK_CUR);	// VOLVIENDO ATRAS AL PUNTERO DEL ARCHIVO
			sentencias++;								// Y SUMANDOLE A LA SENTENCIA PORQUE DESPUES SE LE VUELVE A RESTAR EN EL WHILE...
		}

		if (strcmp(buffer, "OPOK") == 0) {		// SI SALIO TODO BIEN, AVISO AL PLANIF
			if(sentencias){										//Consulto si es la ultima sentencia para enviar el EXEEND
				enviarMensaje(socketPlanificador,"OPOK");		// SI NO ES LA ULTIMA, ENVIO OPOK
			}else{
				enviarMensaje(socketPlanificador,"EXEEND");		// SI LO ES, ENVIO EXEEND
			}
		}
	}

	free(instruccion);

	close(socketPlanificador);
	close(socketCoordinador);
}

void configure_logger(){
	logger = log_create("esi.log","esi",1,LOG_LEVEL_INFO);
}

void crearConfig() {
	config = config_create("../cfg");
}

void setearConfigEnVariables() {
	PUERTOCOORDINADOR = config_get_string_value(config, "Puerto Coordinador");
	IPCOORDINADOR = config_get_string_value(config, "IP Coordinador");
	PUERTOPLANIFICADOR = config_get_string_value(config, "Puerto Planificador");
	IPPLANIFICADOR = config_get_string_value(config, "IP Planificador");;
}

void exitError(int socket,char* error_msg){
	  log_error(logger, error_msg);
	  close(socket);
	  exit(1);
}

void exitErrorBuffer(int socket, char* error_msg, char* buffer){
	if(buffer!=NULL){
		free(buffer);
	}
	exitError(socket,error_msg);
}

void verificarResultado(int socketServidor,int resultado){
	switch(resultado){
		case 0:
			exitError(socketServidor,"Conexion perdida");
			break;
		case -1:
			exitError(socketServidor,"Error al recibir/enviar el mensaje");
			break;
		default:
			break;
	}
}

int conect_to_server(char *ip,char*puerto){
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(ip, puerto, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

	int server_socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (connect(server_socket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		exitError(server_socket, "No se pudo conectar con el servidor");
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar con el servidor"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return server_socket;
}

void recibirHandshake(int socketServidor, char* handshake){
	int size = strlen(handshake) + 1;
	char* buffer = malloc(size);

	int resultado = recv(socketServidor,buffer,size,0);

	verificarResultado(socketServidor,resultado);

	if(strcmp(buffer,handshake)!=0){
		exitErrorBuffer(socketServidor,"Handshake erroneo",buffer);
	}
	log_info(logger,ANSI_COLOR_BOLDGREEN"Handshake recibido correctamente"ANSI_COLOR_RESET);

	free(buffer);
}

void envioIdentificador(int socket) {
	char* identificador = "3";

	if (send(socket, identificador, strlen(identificador)+1, 0) < 0) {
		exitError(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el identificador"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio correctamente el identificador"ANSI_COLOR_RESET);
}

void recibirMensaje(int socketServidor, char* mensaje){
	int size = strlen(mensaje)+1;
	char* recibido = malloc(size+9);
	int resultado;
	resultado = recv(socketServidor, recibido, size, MSG_WAITALL);
	verificarResultado(socketServidor,resultado);

	if(strcmp(recibido,mensaje)!=0){
		exitErrorBuffer(socketServidor,"Mensaje erroneo",recibido);
	}
	strcat(recibido, " recibido");

	log_info(logger,ANSI_COLOR_BOLDCYAN"%s"ANSI_COLOR_RESET, recibido);
	free(recibido);
}

void enviarMensaje(int socketServidor, void* msg){
	int resultado;
	resultado = send(socketServidor,msg, strlen(msg)+1,0);
	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio %s al planificador"ANSI_COLOR_RESET, (char*) msg);
	verificarResultado(socketServidor, resultado);
}

void enviarHeader(int socket, header_t* header) {
	if (send(socket, header, sizeof(header_t), 0) < 0) {
		exitError(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el header"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio el header"ANSI_COLOR_RESET);
}

void enviarClave(int socket, char* clave) {
	if (send(socket, clave, strlen(clave)+1, 0) < 0) {
		exitError(socket, ANSI_COLOR_BOLDRED"No se pudo enviar la clave"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio la clave"ANSI_COLOR_RESET);
}

void enviarValor(int socket, char* valor) {
	int32_t tamanioValor = strlen(valor) +1;

	if (send(socket, &tamanioValor, sizeof(int32_t), 0) < 0) {
		exitError(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el tamanio del valor"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo enviar el tamanio del valor (%d bytes)"ANSI_COLOR_RESET, tamanioValor);

	if (send(socket, valor, tamanioValor, 0) < 0) {
		exitError(socket, ANSI_COLOR_BOLDRED"No se pudo enviar el valor"ANSI_COLOR_RESET);
	}

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se envio el valor %s"ANSI_COLOR_RESET, valor);
}

void ejecutarInstruccion(char* instruccion, int socketCoordinador, int socketPlanificador){
	t_esi_operacion parsed = parse(instruccion);
	header_t* header;
	int32_t tamanioClave;
	if(parsed.valido){
		switch(parsed.keyword){
			case GET:
				tamanioClave = strlen(parsed.argumentos.GET.clave) + 1;
				header = crearHeader(0, tamanioClave);
				enviarHeader(socketCoordinador, header);
				enviarClave(socketCoordinador, parsed.argumentos.GET.clave);

				printf(ANSI_COLOR_BOLDWHITE"GET\tclave: <%s>\n"ANSI_COLOR_RESET, parsed.argumentos.GET.clave);
				free(header);
				break;
			case SET:
				tamanioClave = strlen(parsed.argumentos.SET.clave) + 1;
				header = crearHeader(1, tamanioClave);
				enviarHeader(socketCoordinador, header);
				enviarClave(socketCoordinador, parsed.argumentos.SET.clave);

				enviarValor(socketCoordinador, parsed.argumentos.SET.valor);

				printf(ANSI_COLOR_BOLDWHITE"SET\tclave: <%s>\tvalor: <%s>\n"ANSI_COLOR_RESET, parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
				free(header);
				break;
			case STORE:
				tamanioClave = strlen(parsed.argumentos.STORE.clave) + 1;
				header = crearHeader(2, tamanioClave);
				enviarHeader(socketCoordinador, header);
				enviarClave(socketCoordinador, parsed.argumentos.STORE.clave);

				printf(ANSI_COLOR_BOLDWHITE"STORE\tclave: <%s>\n"ANSI_COLOR_RESET, parsed.argumentos.STORE.clave);
				free(header);
				break;
			default:
				fprintf(stderr, "No pude interpretar <%s>\n", instruccion);
				exit(EXIT_FAILURE);
		}
		destruir_operacion(parsed);

	} else {
		header=crearHeader(4, 0);
		enviarHeader(socketCoordinador, header);
		enviarMensaje(socketPlanificador, "EXEEND");
		free(header);
		exit(EXIT_FAILURE);
	}
}

void administrarID(int socketPlanificador, int socketCoordinador){
	int* recibido = malloc(sizeof(int));
	int resultado;
	resultado = recv(socketPlanificador, recibido, sizeof(int),MSG_WAITALL);
	verificarResultado(socketPlanificador,resultado);
	log_info(logger,ANSI_COLOR_BOLDWHITE"ID ESI %d Recibido"ANSI_COLOR_RESET, *recibido);

	resultado = send(socketCoordinador,recibido,sizeof(int),0);
	verificarResultado(socketCoordinador,resultado);
	log_info(logger,ANSI_COLOR_BOLDWHITE"ID ESI %d Enviado al Coordinador"ANSI_COLOR_RESET, *recibido);
	free(recibido);
}

int cantidadSentencias(FILE* script){
	int sentencias = 0;
	size_t len = 0;
	char* instruccion;
	while(getline(&instruccion,&len,script) != -1){
		sentencias++;
	}
	free(instruccion);
	return sentencias;
}
