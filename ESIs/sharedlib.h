#ifndef SHAREDLIB_H_
#define SHAREDLIB_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct header{
	int codigoOperacion;
	int tamanoClave;
	int tamanoValor;
}header;

typedef struct paquete{
	char* clave;
	char* valor;
}paquete;

void prepararHeader(int, char*,char*, header*);

void empaquetar(char*, char*, char*);

void desempaquetar(char*, header*, paquete*);


#endif /* SHAREDLIB_H_ */
