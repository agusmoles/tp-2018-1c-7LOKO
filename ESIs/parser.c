#include "parser.h"

#define RETURN_ERROR t_esi_operacion ERROR={ .valido = false }; return ERROR
#define GET_KEYWORD "GET"
#define SET_KEYWORD "SET"
#define STORE_KEYWORD "STORE"

#define _CHECK_CLAVE x if(string_length(x) > 40){\
						fprintf(stderr, "La clave <%.40s...> es muy larga\n", (x)); \
						RETURN_ERROR; \
					   }
		

void destruir_operacion(t_esi_operacion op){
	if(op._raw){
		string_iterate_lines(op._raw, (void*) free);
		free(op._raw);
	}
}

t_esi_operacion parse(char* line){
	if(line == NULL || string_equals_ignore_case(line, "")){
		fprintf(stderr, "No pude interpretar una linea vacia\n");
		RETURN_ERROR;
	}

	t_esi_operacion ret = {
		.valido = true
	};

	char* auxLine = string_duplicate(line);
	string_trim(&auxLine);
	char** split = string_n_split(auxLine, 3, " ");

	char* keyword = split[0];
	char* clave = split[1];

	if(clave == NULL){
		fprintf(stderr, "No habia una clave en la linea <%s>\n", (line)); \
		RETURN_ERROR; 
	}
	if(string_length(clave) > 40){
		fprintf(stderr, "La clave <%.40s...> es muy larga\n", (clave)); \
		RETURN_ERROR; 
	}
	
	ret._raw = split;

	if(string_equals_ignore_case(keyword, GET_KEYWORD)){
		ret.keyword = GET;
		ret.argumentos.GET.clave = split[1];
	} else if(string_equals_ignore_case(keyword, STORE_KEYWORD)){
		ret.keyword = STORE;
		ret.argumentos.STORE.clave =  split[1];
	} else if(string_equals_ignore_case(keyword, SET_KEYWORD)){
		ret.keyword = SET;
		ret.argumentos.SET.clave = split[1];
		ret.argumentos.SET.valor = split[2];
	} else {
		fprintf(stderr, "No se encontro el keyword <%s>\n", keyword);
		RETURN_ERROR;
	}

	free(auxLine);
	return ret;
}