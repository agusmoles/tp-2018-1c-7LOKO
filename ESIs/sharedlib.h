#ifndef SHAREDLIB_H_
#define SHAREDLIB_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct header{
	int32_t codigoOperacion;
	int32_t tamanioClave;
}header_t;


header_t* crearHeaderVacio();

header_t* crearHeader(int32_t cod_op, int32_t tamanioClave);

#endif /* SHAREDLIB_H_ */
