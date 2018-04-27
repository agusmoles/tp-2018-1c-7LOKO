/*
 * Copyright (C) 2018 Sistemas Operativos - UTN FRBA. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PARSER_H_
#define PARSER_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <commons/string.h>

	typedef struct {
		bool valido;
		enum {
			GET,
			SET,
			STORE
		} keyword;
		union {
			struct {
				char* clave;
			} GET;
			struct {
				char* clave;
				char* valor;
			} SET;
			struct {
				char* clave;
			} STORE;
		} argumentos;
		char** _raw; //Para uso de la liberación
	} t_esi_operacion;

	/**
	* @NAME: parse
	* @DESC: interpreta una linea de un archivo ESI y
	*		 genera una estructura con el operador interpretado
	* @PARAMS:
	* 		line - Una linea de un archivo ESI
	*/
	t_esi_operacion parse(char* line);
	
	/**
	* @NAME: destruir_operacion
	* @DESC: limpia la operacion generada por el parse
	* @PARAMS:
	* 		op - Una operacion obtenida del parse
	*/
	void destruir_operacion(t_esi_operacion op);

#endif /* PARSER_H_ */