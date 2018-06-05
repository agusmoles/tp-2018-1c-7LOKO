#include "sharedlib.h"
#include <commons/log.h>


header_t* crearHeaderVacio() {
	return (header_t*) malloc(sizeof(header_t));
}

header_t* crearHeader(int32_t cod_op, int32_t tamanioClave) {
	header_t* header = crearHeaderVacio();
	header->codigoOperacion = cod_op;
	header->tamanioClave = tamanioClave;
	printf("Crear hader: COD OP: %d - TAM CLAVE: %d\n", header->codigoOperacion, header->tamanioClave);
	return header;
}


