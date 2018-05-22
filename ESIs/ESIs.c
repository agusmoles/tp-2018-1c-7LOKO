#include "ESIs.h"


int main(){
	configure_logger();
	crearConfig();
	setearConfigEnVariables();
	int socketPlanificador,socketCoordinador;
	FILE* script = fopen("script.esi","r");
	char* instruccion = malloc(PACKAGESIZE);
	size_t len = 0;

	socketPlanificador = conect_to_server(IPPLANIFICADOR,PUERTOPLANIFICADOR);
	recibirHandshake(socketPlanificador,"******PLANIFICADOR HANDSHAKE******");
	socketCoordinador = conect_to_server(IPCOORDINADOR,PUERTOCOORDINADOR);
	recibirHandshake(socketCoordinador,"******COORDINADOR HANDSHAKE******");

	envioIdentificador(socketCoordinador);

	administrarID(socketPlanificador,socketCoordinador);

	while(getline(&instruccion,&len,script) != -1){
		recibirMensaje(socketPlanificador,"EXEOR");

		ejecutarInstruccion(instruccion,socketCoordinador,socketPlanificador);
		recibirMensaje(socketCoordinador,"OPOK");
		enviarMensaje(socketPlanificador,"OPOK");
	}
	recibirMensaje(socketPlanificador, "EXEOR");
	enviarMensaje(socketPlanificador,"EXEEND");

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
	resultado = recv(socketServidor, recibido, size,MSG_WAITALL);
	verificarResultado(socketServidor,resultado);

	if(strcmp(recibido,mensaje)!=0){
		exitErrorBuffer(socketServidor,"Mensaje erroneo",recibido);
	}
	strcat(recibido, " recibido");

	log_info(logger,recibido);
	free(recibido);
}

void enviarMensaje(int socketServidor, char* msg){
	int resultado;
	resultado = send(socketServidor,msg, strlen(msg)+1,0);
	verificarResultado(socketServidor, resultado);
}

char* prepararMensaje(char* operacion, char* clave, char* valor){
	int size = strlen(operacion) + strlen(clave) + 1;
	if(valor!=NULL){
		size = size + strlen(valor) + 1;
	}
	char* mensaje = malloc(size);
	strcpy(mensaje,operacion);
	strcat(mensaje,clave);
	if(valor!=NULL){
		strcat(mensaje,"_");
		strcat(mensaje,valor);
	}
	return mensaje;
}

void ejecutarInstruccion(char* instruccion, int socketCoordinador, int socketPlanificador){
	t_esi_operacion parsed = parse(instruccion);
	if(parsed.valido){
		switch(parsed.keyword){
			case GET:
				enviarMensaje(socketCoordinador,prepararMensaje("GET_",parsed.argumentos.GET.clave,NULL));
				printf("GET\tclave: <%s>\n", parsed.argumentos.GET.clave);
				break;
			case SET:
				enviarMensaje(socketCoordinador,prepararMensaje("SET_",parsed.argumentos.SET.clave,parsed.argumentos.SET.valor));
				printf("SET\tclave: <%s>\tvalor: <%s>\n", parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
				break;
			case STORE:
				enviarMensaje(socketCoordinador,prepararMensaje("STR_",parsed.argumentos.STORE.clave,NULL));
				printf("STORE\tclave: <%s>\n", parsed.argumentos.STORE.clave);
				break;
			default:
				fprintf(stderr, "No pude interpretar <%s>\n", instruccion);
				exit(EXIT_FAILURE);
		}

		destruir_operacion(parsed);
	} else {
		fprintf(stderr, "La linea <%s> no es valida\n", instruccion);
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

