#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct {
  char *nombre;				/* User printable name of the function. */
  void *func;				/* Function to call to do the job. */
  char *doc;				/* Documentation for this function.  */
  int cantidadParametros;
} COMMAND;

int com_pausar(void);
int com_continuar(void);
int com_bloquear(char* clave, int idESI);
int com_desbloquear(char* clave, int idESI);
int com_listar(char* clave);
int com_kill(int idESI);
int com_status();
int com_deadlock();

COMMAND commands[] = {
  { "pausar", *com_pausar, "Pausar planificador", 0},
  { "continuar", *com_continuar, "Continuar planificador", 0},
  { "bloquear", *com_bloquear, "Bloquear ESI", 2},
  { "desbloquear", *com_desbloquear, "Desbloquear ESI", 2},
  { "listar", *com_listar, "Listar procesos esperando recurso", 1},
  { "kll", *com_kill, "Finaliza ESI", 1},
  { "status", *com_status, " ", 1},
  { "deadlock", *com_deadlock," ", 0},
  { (char *)NULL, NULL, (char *)NULL,0 }
};


void main(){
  char * linea;

  while(1){

	linea = readline("ConsolaPlanificador> ");

	if(linea){
		add_history(linea);
		ejecutar_linea(linea);
	}


	if(!strncmp(linea, "exit", 4)) {
       free(linea);
       break;
    }

    free(linea);
  }
}

COMMAND * buscar_command (char *nombre){
  int i;

  for (i = 0; commands[i].nombre; i++)
    if (strcmp (nombre, commands[i].nombre) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

int ejecutar_linea (char *line){
  int i = 0;
  COMMAND *command;
  char *word;

  /* Aislar la palabra "comando" */
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = buscar_command (word);

  if (!command){
      fprintf (stderr, "%s: No existe el comando.\n", word);
      return (-1);
    }

  /* Revisar parametros del comando. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Llamar a la funcion y devolver el resultado */
  return (command->func);
}

/*Esta parte saca los espacios*/
/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char * stripwhite (char *string){
  char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

int com_pausar(void){
	puts("Estas en pausar");
	return 1;
}
int com_continuar(void){
	return 1;
}
int com_bloquear(char* clave, int idESI){
	return 1;
}
int com_desbloquear(char* clave, int idESI){
	return 1;
}
int com_listar(char* clave){
	return 1;
}
int com_kill(int idESI){
	return 1;
}
int com_status(){
	return 1;
}
int com_deadlock(){
	return 1;
}
