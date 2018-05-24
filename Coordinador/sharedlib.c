#include "sharedlib.h"

header* crearHeaderVacio() {
	return (header*) malloc(sizeof(header));
}

header* crearHeader(int cod_op, int tamanioClave) {
	header* header = crearHeaderVacio();
	header->codigoOperacion = cod_op;
	header->tamanioClave = tamanioClave;
	return header;
}

