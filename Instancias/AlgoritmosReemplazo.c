void destruirData(char* data){
	free(data);
}

void destruirEntrada(entrada_t* entrada){
	free(entrada->clave);
	free(entrada);
}

void reemplazarSegun(){

	entrada_t* entradaVieja = list_get(tablaEntradas, posicion);
	entrada_t* entradaViejaSgte = list_get(tablaEntradas, posicionSgte);

	if(strcmp(ALGORITMOREEMPLAZO, "CIRC") == 0) { //CIRCULAR

			while(entradaVieja->clave == entradaViejaSgte->clave){		// no se consideran las entradas NO atomicas
				posicionSgte += 2;
				posicion += 2;
				entradaVieja = list_get(tablaEntradas, posicion);
				entradaViejaSgte = list_get(tablaEntradas, posicionSgte);
			}

			log_info(logger, ANSI_COLOR_BOLDGREEN"Se reemplazo la entrada: Clave %s - Entrada %d - Tamanio Valor %d"ANSI_COLOR_RESET, entradaVieja->clave, entradaVieja->numero, entradaVieja->tamanio_valor);
			list_remove_and_destroy_element(storage, posicion, (void*) destruirData);
			list_remove_and_destroy_element(tablaEntradas, posicion, (void*) destruirEntrada);

			if(posicion < CANTIDADENTRADAS -1)
				posicion ++;
			else
				posicion = 0;
	}

	else if (strcmp(ALGORITMOREEMPLAZO, "LRU") == 0) {	// LRU


	}

	else if (strcmp(ALGORITMOREEMPLAZO, "BSU") == 0) {	// BSU


	}

}



