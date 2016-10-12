/*
 * main_servidor.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "servidor-pokedex.h"

int main(int argc, char **argv) {

	//Validar
	validar();

	//Crear Log
	crearLog();

	//Crear semaforos
	crearSemaforos();

	//Crear servidor
	pthread_create(&pIDServer, NULL, (void *)crearServer, NULL);
	pthread_join(pIDServer, NULL);

}
