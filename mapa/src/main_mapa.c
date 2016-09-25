/*
 * main_mapa.c
 *
 *  Created on: 2/9/2016
 *      Author: utnso
 */

#include "mapa.h"

int main(int argc, char ** argv) {

	signal(SIGUSR2, signal_handler);

	if (chequear_argumentos(argc, TOTAL_ARGS) == -1) {
		return EXIT_FAILURE;
	}

	inicializar_variables();
	inicializar_semaforos();

	nombreMapa = string_duplicate(argv[1]);
	ruta_directorio = string_duplicate(argv[2]);
	leer_metadata_mapa(argv[2]);
	imprimir_metadata();
	cargar_pokenests();
	//imprimir_pokenests();

	socket_servidor = -1;
	//crear_archivo_log();

	pthread_create(&hilo_planificador, NULL, (void *) run_scheduler_thread, NULL);
	pthread_create(&hilo_servidor, NULL, (void *) run_trainer_server, NULL);

	pthread_join(hilo_planificador, NULL);
	pthread_join(hilo_servidor, NULL);

	pthread_detach(hilo_planificador);
	pthread_detach(hilo_servidor);

	destruir_semaforos();
	destruir_variables();

	return EXIT_SUCCESS;
}
