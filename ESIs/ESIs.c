#include "ESIs.h"


int main(){
	configure_logger();
	int socketPlanificador,socketCoordinador;
	FILE* script = fopen("script.txt","r");
	char* leer = malloc(PACKAGESIZE);
	char* instruccion = malloc(PACKAGESIZE);

	socketPlanificador = conect_to_server(IPPLANIFICADOR,PUERTOPLANIFICADOR);
	recibirHandshake(socketPlanificador,"******PLANIFICADOR HANDSHAKE******");
	socketCoordinador = conect_to_server(IPCOORDINADOR,PUERTOCOORDINADOR);
	recibirHandshake(socketCoordinador,"******COORDINADOR HANDSHAKE******");

	envioIdentificador(socketCoordinador);

	do{
		recibirOrdenDeEjecucion(socketPlanificador);
		leer = fgets(instruccion,PACKAGESIZE,script);
		pedirRecursos(socketCoordinador,instruccion);
	}while(leer);

	close(socketPlanificador);
	close(socketCoordinador);


}


void configure_logger(){
	logger = log_create("esi.log","esi",1,LOG_LEVEL_INFO);
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

void pedirRecursos(int socketServidor, char* instruccion){
	int resultado;
	resultado = send(socketServidor,instruccion, strlen(instruccion),0);
	verificarResultado(socketServidor, resultado);
}
