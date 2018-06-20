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

int conectarSocketStatus() {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(IPCOORDINADOR, "8002", &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

	int server_socket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	if (connect(server_socket, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
		_exit_with_error("No se pudo conectar con el servidor");
	}

	log_info(loggerConsola, ANSI_COLOR_BOLDMAGENTA"Se pudo conectar con el Coordinador por el comando status"ANSI_COLOR_RESET);

	freeaddrinfo(serverInfo);

	return server_socket;
}

int cantidad_commands() {
  return sizeof(command_nombre) / sizeof(char *);
}

int ejecutar_consola(){
  char * linea;
  char** args;

  loggerConsola = log_create("loggerConsola.log", "planificador", 1, LOG_LEVEL_INFO);
  socketStatus = conectarSocketStatus();
	
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

	int* IDESI = malloc(sizeof(int));

	*IDESI = atoi(args[2]);

	cliente* ESI = buscarESI(IDESI);

	if (ESI == NULL) {
		log_error(loggerConsola, ANSI_COLOR_BOLDRED"No existe el ESI en el Sistema."ANSI_COLOR_RESET);

		if (!dictionary_has_key(diccionarioClaves, args[1])) {		// SI NINGUN ESI LA TIENE, LA TOMA EL SISTEMA
			*IDESI = -1;

			dictionary_put(diccionarioClaves, args[1], IDESI);

			log_info(loggerConsola, ANSI_COLOR_BOLDCYAN"La clave %s fue tomada por el Sistema"ANSI_COLOR_RESET, args[1]);
		}
	} else {
		if (dictionary_has_key(diccionarioClaves, args[1])) {		// SI ALGUIEN YA TIENE EL RECURSO ASIGNADO, BLOQUEO AL ESI

			int* IDEsiQueTieneLaClaveTomada = dictionary_get(diccionarioClaves, args[1]);

			if (esiEstaEnBloqueados(ESI)) {		// SI YA ESTABA BLOQUEADO, NO HACE NADA
				log_error(loggerConsola, ANSI_COLOR_BOLDRED"El ESI %d ya se encontraba bloqueado por la clave %s"ANSI_COLOR_RESET, ESI->identificadorESI, ESI->recursoSolicitado);
			}


			if (esiEstaEjecutando(ESI)) { // SI EL ESI ESTABA EJECUTANDO, TENGO QUE DESALOJARLO Y ORDENAR A OTRO A EJECUTAR
				sem_wait(&desalojoComandoBloquear);
				ESI->desalojoPorComandoBloquear = 1;
				sem_post(&desalojoComandoBloquear);

				strcpy(ESI->recursoSolicitado, args[1]);	// LE ASIGNO LA CLAVE POR LA QUE SE BLOQUEO

				log_error(loggerConsola, ANSI_COLOR_BOLDRED"La clave %s ya estaba tomada por el ESI %d. Se bloqueo al ESI %d "ANSI_COLOR_RESET, args[1], *IDEsiQueTieneLaClaveTomada, *IDESI);
			}

			for (int i=0; i<list_size(listos); i++) {
				cliente* ESIListo = list_get(listos, i);

				if (ESIListo->identificadorESI == ESI->identificadorESI) {		// SI ESTA EN LISTOS, LO SACO Y PASO A BLOQUEADO
					strcpy(ESI->recursoSolicitado, args[1]);

					sem_wait(&mutexListos);
					list_remove(listos, i);
					sem_post(&mutexListos);

					sem_wait(&mutexBloqueados);
					list_add(bloqueados, ESI);
					sem_post(&mutexBloqueados);

					log_error(loggerConsola, ANSI_COLOR_BOLDRED"La clave %s ya estaba tomada por el ESI %d. Se bloqueo al ESI %d "ANSI_COLOR_RESET, args[1], *IDEsiQueTieneLaClaveTomada, *IDESI);
				}
			}

			free(IDESI);
		} else {		// SI NO LO TIENEN ASIGNADO, SE LO ASIGNO AL ESI

			dictionary_put(diccionarioClaves, args[1], IDESI);

			log_info(loggerConsola, ANSI_COLOR_BOLDCYAN"El ESI %d tomo efectivamente la clave %s"ANSI_COLOR_RESET, *IDESI, args[1]);

		}
	}

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

	if (dictionary_has_key(diccionarioClaves, args[1])) {
		desbloquearESI(args[1]);

		if (!hayEsisBloqueadosEsperandoPor(args[1])) {		// SI NO HAY MAS ESIS BLOQUEADOS POR LA CLAVE, LA LIBERO
			int* IDESI = dictionary_remove(diccionarioClaves, args[1]);
			free(IDESI);
		}
	} else {
		log_info(loggerConsola, ANSI_COLOR_BOLDWHITE"La clave %s no se encontraba en el sistema\n"ANSI_COLOR_RESET, args[1]);
	}

	liberar_parametros(args);
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


	int* IDESI = malloc(sizeof(int));
	*IDESI = atoi(args[1]);
	cliente* ESI = buscarESI(IDESI);

	for (int i=0; i<list_size(bloqueados); i++) {
		cliente* ESIBloqueado = list_get(bloqueados, i);

		if (ESIBloqueado->identificadorESI == ESI->identificadorESI) {
			sem_wait(&mutexBloqueados);
			list_remove(bloqueados, i);
			sem_post(&mutexBloqueados);

			sem_wait(&mutexFinalizados);
			list_add(finalizados, ESI);
			sem_post(&mutexFinalizados);

			ESIABuscarEnDiccionario = malloc(sizeof(int));
			*ESIABuscarEnDiccionario = *IDESI;	// SETEO LA VARIABLE GLOBAL

			sem_wait(&mutexDiccionarioClaves);
			dictionary_iterator(diccionarioClaves, (void*) eliminarClavesTomadasPorEsiFinalizado); // REMUEVO LAS CLAVES TOMADAS POR EL ESI A FINALIZAR
			sem_post(&mutexDiccionarioClaves);

			free(ESIABuscarEnDiccionario);
		}
	}


	if (esiEstaEjecutando(ESI)) { // SI EL ESI ESTABA EJECUTANDO, TENGO QUE ABORTARLO Y ORDENAR A OTRO A EJECUTAR
		sem_wait(&desalojoComandoKill);
		ESI->desalojoPorComandoKill = 1;
		sem_post(&desalojoComandoKill);
	}

	for (int i=0; i<list_size(listos); i++) {
		cliente* ESIListo = list_get(listos, i);

		if (ESIListo->identificadorESI == ESI->identificadorESI) {		// SI ESTA EN LISTOS, LO SACO Y FINALIZO
			sem_wait(&mutexListos);
			list_remove(listos, i);
			sem_post(&mutexListos);

			sem_wait(&mutexFinalizados);
			list_add(finalizados, ESI);
			sem_post(&mutexFinalizados);

			ESIABuscarEnDiccionario = malloc(sizeof(int));
			*ESIABuscarEnDiccionario = *IDESI;	// SETEO LA VARIABLE GLOBAL

			sem_wait(&mutexDiccionarioClaves);
			dictionary_iterator(diccionarioClaves, (void*) eliminarClavesTomadasPorEsiFinalizado); // REMUEVO LAS CLAVES TOMADAS POR EL ESI A FINALIZAR
			sem_post(&mutexDiccionarioClaves);

			free(ESIABuscarEnDiccionario);
		}
	}

	free(IDESI);

	liberar_parametros(args);

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

	int* tamanioClave = malloc(sizeof(int));
	int* tamanioValor = malloc(sizeof(int));
	char* bufferValor;
	int* instancia = malloc(sizeof(int));

	*tamanioClave = strlen(args[1]) + 1;

	/*********************** PROCEDIMIENTO EXPLICADO EN PROTOCOLO.TXT ****************************/

	if (send(socketStatus, tamanioClave, sizeof(int), 0) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo enviar el tamaño de la clave en status"ANSI_COLOR_RESET);
	}

	if (send(socketStatus, args[1], *tamanioClave, 0) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo enviar la clave en status"ANSI_COLOR_RESET);
	}

	if (recv(socketStatus, tamanioValor, sizeof(int), 0) < 0) {
		_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el tamaño de valor en status"ANSI_COLOR_RESET);
	}

	if (*tamanioValor == -1) {		// SI NO HAY VALOR, QUE RECIBA -1 EL TAMANIO Y DESPUES LA INSTANCIA
		log_error(loggerConsola, ANSI_COLOR_BOLDRED"La clave no existe o no tiene valor guardado en ninguna instancia"ANSI_COLOR_RESET);

		if (recv(socketStatus, instancia, sizeof(int), 0) < 0) {
			log_error(loggerConsola, ANSI_COLOR_BOLDRED"El valor se guardaria en la instancia %d"ANSI_COLOR_RESET, *instancia);
		}


	} else {

		bufferValor = malloc(*tamanioValor);

		if (recv(socketStatus, bufferValor, *tamanioValor, 0) < 0) {
			_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir el valor en status"ANSI_COLOR_RESET);
		}

		log_info(loggerConsola, ANSI_COLOR_BOLDMAGENTA"El valor de la clave %s es %s"ANSI_COLOR_RESET, args[1], bufferValor);

		if (recv(socketStatus, instancia, sizeof(int), 0) < 0) {
			_exit_with_error(ANSI_COLOR_BOLDRED"No se pudo recibir la instancia en status"ANSI_COLOR_RESET);
		}

		log_info(loggerConsola, ANSI_COLOR_BOLDMAGENTA"El valor esta guardado en la instancia %d"ANSI_COLOR_RESET, *instancia);

		free(bufferValor);
	}

	listar(args[1]);

	free(tamanioClave);
	free(tamanioValor);
	free(instancia);

	liberar_parametros(args);

	return 1;
}

int com_deadlock(char **args){
	if(args[1] != NULL){
		return error_no_lleva_parametros(args);
	}

	t_list* deadlock = list_create();
	int* IDESI;
	int* IDESISiguiente;
	cliente* ESI, *ESIQueTieneElRecurso;

	for(int i=0; i<list_size(bloqueados); i++) {
		ESI = list_get(bloqueados, i);

		list_add(deadlock, ESI);

		IDESI = dictionary_get(diccionarioClaves, ESI->recursoSolicitado);

		ESIQueTieneElRecurso = buscarESI(IDESI);

		if(!esiEstaEnBloqueados(ESIQueTieneElRecurso)) {
			// SI YA NO ESTA BLOQUEADO, NO HAY DEADLOCK
			list_remove(deadlock, 0);
		} else {
			IDESISiguiente = dictionary_get(diccionarioClaves, ESIQueTieneElRecurso->recursoSolicitado);

			if (*IDESISiguiente == *IDESI) {
				list_add(deadlock, ESIQueTieneElRecurso);
			}
		}
	}

	for(int i=0; i<list_size(deadlock); i++) {
		cliente* esi = list_get(deadlock, i);

		printf(ANSI_COLOR_BOLDWHITE"ESI %d\n"ANSI_COLOR_RESET, esi->identificadorESI);
	}

	list_destroy(deadlock);
	liberar_parametros(args);

	return 1;
}

cliente* buscarESIEnBloqueados(int* IDESI) {
	cliente* ESI;

	for (int i=0; i<list_size(bloqueados); i++) {
		ESI = list_get(bloqueados, i);

		if (ESI->identificadorESI == *IDESI) {
			return ESI;
		}
	}

	return NULL;
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
