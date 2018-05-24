#include "sharedlib.h"

void prepararHeader(int operacion , char* clave ,char* valor , header* encabezado){ //Arma el header
	encabezado->codigoOperacion = operacion;
	encabezado->tamanoClave = strlen(clave);
	if(valor != NULL){
		encabezado->tamanoValor = strlen(valor);
	}else{
		encabezado->tamanoValor = 0;
	}
}

void desempaquetar(char* buffer, header* encabezado, paquete* paquetito){
	char* auxiliarIndicador = buffer;
	paquetito->clave = malloc(encabezado->tamanoClave);
	memcpy(paquetito->clave,auxiliarIndicador,encabezado->tamanoClave);
	if(encabezado->codigoOperacion == 1){
		auxiliarIndicador += encabezado->tamanoClave;
		paquetito->valor = malloc(encabezado->tamanoValor);
		memcpy(paquetito->valor,auxiliarIndicador,encabezado->tamanoValor);
	}else{
		paquetito->valor = NULL;
	}
	free(auxiliarIndicador);
}
