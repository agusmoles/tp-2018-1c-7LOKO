#include "sharedlib.h"

void prepararHeader(int operacion , char* clave ,char* valor , header* encabezado){
	encabezado->codigoOperacion = operacion;
	encabezado->tamanoClave = sizeof(clave);
	if(valor != NULL){
		encabezado->tamanoValor= sizeof(valor);
	}else{
		encabezado->tamanoValor = 0;
	}
}

void empaquetar(char* clave, char* valor , char* buffer){
	int size = strlen(clave) + strlen(valor) + 1;
	buffer = malloc(size);
	strcpy(buffer,clave);
	if(valor != NULL){
		strcat(buffer,valor);
	}
}

void desempaquetar(char* buffer, header* encabezado, paquete* paquetito){
	char* auxiliarIndicador = buffer;
	paquetito->clave = malloc(encabezado->tamanoClave);
	stcncpy(paquetito->clave,auxiliarIndicador,encabezado->tamanoClave);
	if(encabezado->codigoOperacion == 1){
		auxiliarIndicador =+ encabezado->tamanoClave;
		paquetito->valor = malloc(encabezado->tamanoValor);
		strncpy(paquetito->valor,auxiliarIndicador,encabezado->tamanoValor);
	}else{
		paquetito->valor = NULL;
	}
}
