#include "ESIs.h"


int main(){
	configure_logger();
	crearConfig();
	setearConfigEnVariables();
	int socketPlanificador,socketCoordinador;
	FILE* script = fopen("script.esi","r");
	char* instruccion = malloc(PACKAGESIZE);;
	size_t len = 0;

	socketPlanificador = conect_to_server(IPPLANIFICADOR,PUERTOPLANIFICADOR);
	recibirHandshake(socketPlanificador,"******PLANIFICADOR HANDSHAKE******");
	socketCoordinador = conect_to_server(IPCOORDINADOR,PUERTOCOORDINADOR);
	recibirHandshake(socketCoordinador,"******COORDINADOR HANDSHAKE******");

	envioIdentificador(socketCoordinador);

	while(getline(&instruccion,&len,script) != -1){
		enviarMensaje(socketPlanificador,"EXERQ");
		recibirOrdenDeEjecucion(socketPlanificador);

		ejecutarInstruccion(instruccion,socketCoordinador,socketPlanificador);

		enviarMensaje(socketCoordinador,instruccion);
	}
	close(socketPlanificador);
	close(socketCoordinador);
}

void ejecutarInstruccion(char* instruccion, int socketCoordinador, int socketPlanificador){
	t_esi_operacion parsed = parse(instruccion);
	if(parsed.valido){
		switch(parsed.keyword){
			case GET:
				instruccionGet(&parsed, socketCoordinador,socketPlanificador);
				printf("GET\tclave: <%s>\n", parsed.argumentos.GET.clave);
				break;
			case SET:
				instruccionSet(&parsed, socketCoordinador, socketPlanificador);
				printf("SET\tclave: <%s>\tvalor: <%s>\n", parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
				break;
			case STORE:
				instruccionStore(&parsed,socketCoordinador,socketPlanificador);
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

void recibirOrdenDeEjecucion(int socketServidor){
	char* ejecutar = malloc(1);
	int resultado;
	resultado = recv(socketServidor, ejecutar, 1,MSG_WAITALL);
	verificarResultado(socketServidor,resultado);
	if(strcmp(ejecutar,"1")!=0){
			exitErrorBuffer(socketServidor,"Mensaje erroneo",ejecutar);
		}
	log_info(logger,"Orden de ejecucion linea de script recibida");
}

void enviarMensaje(int socketServidor, char* msg){
	int resultado;
	resultado = send(socketServidor,msg, strlen(msg)+1,0);
	verificarResultado(socketServidor, resultado);
}

void instruccionGet(t_esi_operacion* operacion,int coordinador,int planificador){
	int size = strlen("GET_") + strlen(operacion->argumentos.GET.clave) +1;
	char* instruccion = malloc(size);
	strcpy(instruccion,"GET_");
	strcat(instruccion,operacion->argumentos.GET.clave);
	enviarMensaje(coordinador,instruccion);
}

void instruccionSet(t_esi_operacion* operacion,int coordinador,int planificador){
	int size = strlen("SET_") + strlen(operacion->argumentos.SET.clave) + strlen(operacion->argumentos.SET.valor)+2;
	char* instruccion = malloc(size);
	strcpy(instruccion,"SET_");
	strcat(instruccion,operacion->argumentos.SET.clave);
	strcat(instruccion,"_");
	strcat(instruccion,operacion->argumentos.SET.valor);
	enviarMensaje(coordinador,instruccion);
}

void instruccionStore(t_esi_operacion* operacion,int coordinador,int planificador){
	int size = strlen("STR_") + strlen(operacion->argumentos.STORE.clave) +1;
	char* instruccion = malloc(size);
	strcpy(instruccion,"STR_");
	strcat(instruccion,operacion->argumentos.STORE.clave);
	enviarMensaje(coordinador,instruccion);
}
