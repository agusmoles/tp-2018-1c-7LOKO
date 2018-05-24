#ifndef SHAREDLIB_H_
#define SHAREDLIB_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct header{
	int codigoOperacion;
	int tamanioClave;
}header;


header* crearHeaderVacio();

header* crearHeader(int cod_op, int tamanioClave);

#endif /* SHAREDLIB_H_ */
