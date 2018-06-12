#include <stdio.h>
#include "instancia.h"

#define CIRCULAR 0
#define LRU 1
#define BSU 2

int index = 0;
// t_list* puntero = &list_get(tablaEntradas, index);


void reemplazarSegun(int algoritmo, entrada_t* entrada){

	switch(algoritmo){

		case "CIRCULAR":		// Circular

			if(index < CANTIDADENTRADAS -1){
				list_replace_and_destroy_element(tablaEntradas, index, entrada, );
				index ++;
			}else{
				list_replace_and_destroy_element(tablaEntradas, index, entrada, );
				index = 0;
			}









	}

}


int main(){
	return 0;
}
