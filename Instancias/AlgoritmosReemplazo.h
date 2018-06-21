#ifndef ALGORITMOSREEMPLAZO_H_
#define ALGORITMOSREEMPLAZO_H_

#include <stdio.h>
#include <commons/log.h>
#include "../Colores.h"

typedef struct Entrada{
	char* clave;
	int numero;
	int tamanio_valor;
}entrada_t ;

#define CIRCULAR 0
#define LRU 1
#define BSU 2

int posicion = 0;
int posicionSgte = 1;

t_log* logger;
char* ALGORITMOREEMPLAZO;
char** storage;

void destruirEntrada(entrada_t*);
void destruirData(char*);
void reemplazarSegun();


#endif /* ALGORITMOSREEMPLAZO_H_ */
