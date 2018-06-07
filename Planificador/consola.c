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

int ejecutar_consola(){
  char * linea;
  char** args;
	
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
		printf(ANSI_COLOR_BOLDWHITE"Planificador pausado\n"ANSI_COLOR_RESET);
		sem_wait(&pausado);
		liberar_parametros(args);
		return 1;
	}
}

int com_continuar(char **args){
	if(args[1] != NULL){
		return error_no_lleva_parametros(args);
	}else{
		sem_post(&pausado);
		printf(ANSI_COLOR_BOLDWHITE"Planificador despausado\n"ANSI_COLOR_RESET);
		liberar_parametros(args);
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

	char* nombreESI = malloc(7);	// 7 PORQUE ES "ESI ID", DEJO ESPACIO PARA ESIS DE MAS DE DOS CIFRAS

	strcpy(nombreESI, "ESI ");

	strcat(nombreESI, args[2]);	// CONCATENO "ESI" CON EL ID...

	char* clave = malloc(strlen(args[1]) + 1);

	strcpy(clave, args[1]);

	int* IDESI = malloc(sizeof(int));

	*IDESI = atoi(args[2]);

	if (dictionary_has_key(diccionarioClaves, clave)) {
		// TENGO QUE BLOQUEAR AL ESI BLA BLA BLA

		bloquearESI(clave, IDESI);

		char* IDEsiQueTieneLaClaveTomada = dictionary_get(diccionarioClaves, clave);

		log_error(loggerConsola, ANSI_COLOR_BOLDRED"La clave %s ya estaba tomada por el %s. Se bloqueo al %s"ANSI_COLOR_RESET, clave, IDEsiQueTieneLaClaveTomada, nombreESI);

//					informarClaveTomada(); AL COORDINADOR
	} else {

		dictionary_put(diccionarioClaves, clave, nombreESI);

		log_info(loggerConsola, ANSI_COLOR_BOLDCYAN"El %s tomo efectivamente la clave %s"ANSI_COLOR_RESET, nombreESI, clave);

//					informarClaveTomada(); AL COORDINADOR
	}

	free(IDESI);
	free(clave);
	liberar_parametros(args);

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

//	if (dictionary_has_key(diccionarioClaves, args[1])) {
//		dictionary_remove(diccionarioClaves, args[1]);
//		desbloquearESI(args[1]);
//
//	} else {
//		printf(ANSI_COLOR_BOLDWHITE"La clave %s no se encontraba en el sistema\n"ANSI_COLOR_RESET, args[1]);
//	}
//
//	liberar_parametros(args);
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

	listar(args[1]);

	liberar_parametros(args);

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

	// NO OLVIDARSE DE LIBERAR PARAMETROS

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

	// NO OLVIDARSE DE LIBERAR PARAMETROS

	puts("Estas en status");
	return 1;
}

int com_deadlock(char **args){
	if(args[1] != NULL){
		return error_no_lleva_parametros(args);
	}

	// NO OLVIDARSE DE LIBERAR PARAMETROS

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
