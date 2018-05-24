#include "sharedlib.h"
#include <commons/log.h>


header* crearHeaderVacio() {
	return (header*) malloc(sizeof(header));
}

header* crearHeader(int cod_op, int tamanioClave) {
	header* header = crearHeaderVacio();
	header->codigoOperacion = cod_op;
	header->tamanioClave = tamanioClave;
	printf("Crear hader: COD OP: %d - TAM CLAVE: %d\n", header->codigoOperacion, header->tamanioClave);
	return header;
}


