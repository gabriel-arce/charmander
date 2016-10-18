/*
 * shared_serializacion.h
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#ifndef SHARED_SERIALIZACION_H_
#define SHARED_SERIALIZACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <commons/string.h>

typedef struct {
	uint8_t identificador;
	uint32_t tamanio;
}__attribute__((packed)) t_header;

typedef struct {
	char* nombreArchivo;
	char* nombre;
	int nivel;
} t_pokemon;

t_header * crear_header(int id, int size);

void * serializar_header(int id, int size);
t_header * deserializar_header(void * buffer);

void * serializarPokemon(t_pokemon * pokemon);
t_pokemon * deserializarpokemon(void* pokemonSerializado);

#endif /* SHARED_SERIALIZACION_H_ */
