#include <stdio.h>
#include "instancia.h"

#define CIRCULAR 0
#define LRU 1
#define BSU 2

int posicion = 0;
int posicionSgte = 1;

void destruirData(data_t* data){
	free(data->valor);
	free(data);
}

void destruirEntrada(entrada_t* entrada){
	free(entrada->clave);
	free(entrada);
}

void reemplazarSegun(int algoritmo, entrada_t* entrada){

	entrada_t* entradaVieja = list_get(tablaEntradas, posicion);
	entrada_t* entradaViejaSgte = list_get(tablaEntradas, posicionSgte);

	switch(algoritmo){

		case 0:		// CIRCULAR

			while(entradaVieja->clave == entradaViejaSgte->clave){		// no se consideran las entradas NO atomicas
				posicionSgte += 2;
				posicion += 2;
				entradaVieja = list_get(tablaEntradas, posicion);
				entradaViejaSgte = list_get(tablaEntradas, posicionSgte);
			}

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se reemplazo la entrada: Clave %s - Entrada %d - Tamanio Valor %d"ANSI_COLOR_RESET, entradaVieja->clave, entradaVieja->numero, entradaVieja->tamanio_valor);;
			list_remove_and_destroy_element(listaStorage, posicion, (void*) destruirData);
			list_replace_and_destroy_element(tablaEntradas, posicion, entrada, (void*) destruirEntrada);

			if(posicion < CANTIDADENTRADAS -1)
				posicion ++;
			else
				posicion = 0;

			break;


		case 1:		// LRU
			break;


		case 2:		// BSU
			break;


		default:
			break;

	}

}



