#include "consola.h"

char *command_nombre[] = {
  "pausar",
  "continuar",
  "bloquear",
  "desbloquear",
  "listar",
  "kill",
  "status",
  "deadlock",
  "man"
};

char *command_docu[] = {
		"Pausar planificador",
		"Continuar planificador",
		"Bloquea el proceso ESI",
		"Desbloquea el proceso ESI",
		"Lista procesos encolados esperando al recurso",
		"Finaliza el proceso",
		"Consultar estado",
		"Analizar los deadlocks del sistema y a que ESI estan asociados",
		"Breve documentacion de cada comando"
};

int (*com_func[]) (char **) = {
	&com_pausar,
	&com_continuar,
	&com_bloquear,
	&com_desbloquear,
	&com_listar,
	&com_kill,
	&com_status,
	&com_deadlock,
	&com_man
};

int cantidad_commands() {
  return sizeof(command_nombre) / sizeof(char *);
}

//CONEXION AL PLANIFICADOR
void _exit_with_error(int socket, char* mensaje) {
	close(socket);
	log_error(logger, mensaje);
	exit(1);
}

void configurarLogger() {
	logger = log_create("consola.log", "cliente", 1, LOG_LEVEL_INFO);
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

	log_info(logger, ANSI_COLOR_BOLDGREEN"Se pudo conectar con el planificador"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return server_socket;
}

void reciboHandshake(int socket) {
	char* handshake = "******PLANIFICADOR HANDSHAKE******";
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

int main(){
  char * linea;
  char** args;

  configurarLogger();
  int socket = conectarSocket();

  reciboHandshake(socket);
	
  while(1){

	linea = readline(ANSI_COLOR_BOLDGREEN "ConsolaPlanificador> " ANSI_COLOR_RESET);

	if(!strncmp(linea, "exit", 4)) { //Si es 'exit' termina la consola
       free(linea);
       break;
    }

	if(linea){
		add_history(linea);
		args = string_split(linea, " "); //Separo por espacios => la posicion 0 es el comando
		ejecutar_linea(args);
	}

	free(args);
    free(linea);
  }

  return EXIT_SUCCESS;
}

void liberar_parametros(char **args){
	for(int j=0; args[j]; j++){
		   free(args[j]);
	}
}

int ejecutar_linea (char **args){
  int i;
  int cantidad = cantidad_commands();

  for (i = 0; i < cantidad; i++){
	  if(strcmp (args[0], command_nombre[i]) == 0){
	      	return (*com_func[i]) (args);
	      }
  }

  //Si no esta en la lista de commands
  printf ("%s: No existe el comando.\n", args[0]);

  liberar_parametros(args);
  return -1;
}

int error_no_lleva_parametros(char ** args){
	printf(ANSI_COLOR_RED "ERROR: '%s' no recibe parametros\n" ANSI_COLOR_RESET, args[0]);
	liberar_parametros(args);
	return -1;
}

int error_sobran_parametros(char ** args){
	printf(ANSI_COLOR_RED "ERROR: '%s' tiene parametros de mas\n" ANSI_COLOR_RESET, args[0]);
	liberar_parametros(args);
	return -1;
}

int error_faltan_parametros(char **args){
	printf(ANSI_COLOR_RED"ERROR: '%s' faltan parametros\n"ANSI_COLOR_RESET, args[0]);
	liberar_parametros(args);
	return -1;
}

int com_pausar(char **args){
	if(args[1] != NULL){
		return error_no_lleva_parametros(args);
	}else{
		puts("Estas en pausar");
		return 1;
	}
}

int com_continuar(char **args){
	if(args[1] != NULL){
		return error_no_lleva_parametros(args);
	}else{
		puts("Estas en continuar");
		return 1;
	}
}

int com_bloquear(char **args){
	if(args[1] == NULL || args[2] == NULL){
		return error_faltan_parametros(args);
	}

	for(int k=3; args[k]; k++){
		if(args[k] != NULL)
			return error_sobran_parametros(args);
	}

	puts("Estas en bloquear");
	return 1;

}

int com_desbloquear(char **args){
	if(args[1] == NULL){
		return error_faltan_parametros(args);
	}

	for(int k=2; args[k]; k++){
		if(args[k] != NULL)
			return error_sobran_parametros(args);
	}

	puts("Estas en desbloquear");
	return 1;

}

int com_listar(char **args){
	if(args[1] == NULL){
		return error_faltan_parametros(args);
	}

	for(int k=2; args[k]; k++){
		if(args[k] != NULL)
			return error_sobran_parametros(args);
	}

	puts("Estas en listar");
	return 1;
}

int com_kill(char **args){
	if(args[1] == NULL){
		return error_faltan_parametros(args);
	}

	for(int k=2; args[k]; k++){
		if(args[k] != NULL)
			return error_sobran_parametros(args);
	}

	puts("Estas en kill");
	return 1;
}

int com_status(char **args){
	if(args[1] == NULL){
		return error_faltan_parametros(args);
	}

	for(int k=2; args[k]; k++){
		if(args[k] != NULL)
			return error_sobran_parametros(args);
	}

	puts("Estas en status");
	return 1;
}

int com_deadlock(char **args){
	if(args[1] != NULL){
		return error_no_lleva_parametros(args);
	}

	puts("Estas en deadlock");
	return 1;
}

int com_man(char **args){
	if(args[1] == NULL){
		return error_faltan_parametros(args);
	}else{
		for (int i = 0; i < cantidad_commands(); i++){
		    if(strcmp (args[1], command_nombre[i]) == 0){
		    	puts(command_docu[i]);
		    	liberar_parametros(args);
		    	return 1;
		    }
		}
		//Si no esta en la lista de commands
		printf ("%s: No existe el comando.\n", args[1]);
		liberar_parametros(args);
		return 1;
	}
}
