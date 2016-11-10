/*
 * mapa-utils.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "mapa.h"

void imprimir_metadata() {
	printf("\n<<<METADA DEL MAPA>>>\n");
	printf("tiempo de chequeo de DL: %d\n", metadata->tiempoChequeoDeadlock);
	printf("batalla: %d\n", metadata->batalla);

	if (metadata->planificador->algth == RR) {
		printf("algoritmo: Round Robin , con quantum: %d\n", metadata->planificador->quantum);
	} else {
		printf("algoritmo: SRDF\n");
	}

	printf("retardo: %d\n", metadata->planificador->retardo_turno);
	printf("IP: %s\n", metadata->ip);
	printf("puerto: %d\n", metadata->puerto);
	printf("\n");
}

void inicializar_semaforos() {

	void on_error_sem(sem_t * s) {
		if (s == NULL)
			exit(EXIT_FAILURE);
	}

	pthread_mutex_init(&mutex_gui, 0);
	pthread_mutex_init(&mutex_servidor, 0);
	pthread_mutex_init(&mutex_planificador_turno, 0);
	pthread_mutex_init(&mutex_cola_listos, 0);
	pthread_mutex_init(&mutex_cola_bloqueados, 0);
	pthread_mutex_init(&mutex_cola_prioridadSRDF, 0);
	pthread_mutex_init(&mutex_entrenadores, 0);
	pthread_mutex_init(&mutex_log, 0);
	pthread_mutex_init(&mutex_pokenests, 0);
	semaforo_de_listos = crearSemaforo(0);
	on_error_sem(semaforo_de_listos);
	semaforo_de_bloqueados = crearSemaforo(0);
	on_error_sem(semaforo_de_bloqueados);
}

void destruir_semaforos() {
	pthread_mutex_destroy(&mutex_gui);
	pthread_mutex_destroy(&mutex_servidor);
	pthread_mutex_destroy(&mutex_planificador_turno);
	pthread_mutex_destroy(&mutex_cola_bloqueados);
	pthread_mutex_destroy(&mutex_cola_listos);
	pthread_mutex_destroy(&mutex_cola_prioridadSRDF);
	pthread_mutex_destroy(&mutex_entrenadores);
	pthread_mutex_destroy(&mutex_log);
	pthread_mutex_destroy(&mutex_pokenests);
	destruirSemaforo(semaforo_de_listos);
	destruirSemaforo(semaforo_de_bloqueados);
}

void inicializar_variables() {
	entrenadores_conectados = list_create();
	cola_de_listos = list_create();
	lista_de_pokenests = list_create();
	quantum_actual = 0;
	keep_running = false;
	entrenador_corriendo = NULL;
	items_mapa = list_create();
	cola_de_bloqueados = list_create();
}

void destruir_variables() {
	list_destroy_and_destroy_elements(entrenadores_conectados, (void *) entrenador_destroyer);
	list_destroy(cola_de_listos);
	list_destroy_and_destroy_elements(lista_de_pokenests, (void *) pokenest_destroyer);
	list_destroy_and_destroy_elements(items_mapa, (void *) item_destroyer);

	list_destroy(cola_de_bloqueados);

	free(nombreMapa);
	free(ruta_directorio);
}

void entrenador_destroyer(t_entrenador * e) {
	free(e->nombre_entrenador);
	free(e->posicion);
	free(e->pokemonesCapturados);
}

void pokenest_destroyer(t_pokenest * r) {
	free(r->nombre);

	void pkm_destroyer(t_pkm * p) {
		free(p->nombre);
		free(p->nombreArchivo);
	}
	list_destroy_and_destroy_elements(r->pokemones, (void *) pkm_destroyer);
	free(r->posicion);
	free(r->tipo);
	free(r);
}

void item_destroyer(void * item) {

	ITEM_NIVEL * i = (ITEM_NIVEL *) item;

	BorrarItem(items_mapa, i->id);
}

void pokemon_remover(t_pkm * pkm, t_list * list) {
	int i;

	for( i = 0; i < list_size(list); i++ ) {
		t_pkm * p = list_get(list, i);

		if (string_equals_ignore_case(p->nombreArchivo, pkm->nombreArchivo))
			break;
	}

	list_remove(list, i);
}

void imprimir_pokenests() {
	void pknst_printer(t_pokenest * pknst) {
		printf("\nPokenest : %s\n", pknst->nombre);
		printf("Ubicacion: ( %d; %d )\n", pknst->posicion->x, pknst->posicion->y);
		printf("Pokemons: \n");
		void pkm_printer(t_pkm * pkm) {
			printf("%s\n", pkm->nombreArchivo);
		}
		list_iterate(pknst->pokemones, (void *) pkm_printer);
	}
	list_iterate(lista_de_pokenests, (void *) pknst_printer);
}

bool esta_en_pokenest(t_entrenador * entrenador) {

	return (bool) ((entrenador->posicion->x == entrenador->posicionObjetivo->x)
			&& (entrenador->posicion->y == entrenador->posicionObjetivo->y));
}

t_pkm * recibirPokemon(int socket){

	t_header * header_in = recibir_header(socket);

	void* buffer_in = malloc(header_in->tamanio);

	if (recv(socket,buffer_in,header_in->tamanio,0) < 0) {
		free(buffer_in);
		return NULL;
	}

	t_pkm * pokemon = deserializarPokemon(buffer_in);

	free(header_in);
	free(buffer_in);

	return pokemon;
}

t_pkm * deserializarPokemon(void* pokemonSerializado){

	t_pkm * pokemon = malloc(sizeof(t_pkm));

	int nombreSize;
	int nombreArchivoSize;

	memcpy(&nombreSize, pokemonSerializado, 4);
	memcpy(&nombreArchivoSize, pokemonSerializado + 4, 4);

	pokemon->nombre = (char *) malloc(sizeof(char) * nombreSize);
	pokemon->nombreArchivo = (char *) malloc(sizeof(char) * nombreArchivoSize);

	memcpy(&pokemon->nivel, pokemonSerializado + 8, 4);
	memcpy(pokemon->nombre, pokemonSerializado + 12, nombreSize);
	memcpy(pokemon->nombreArchivo, pokemonSerializado + 12 + nombreSize,
			nombreArchivoSize);

	return pokemon;
}

t_entrenador * buscar_entrenador_por_simbolo(char symbol_expected) {
	t_entrenador * entrenador = NULL;

	bool find_trainer(t_entrenador * s_t) {
		return (s_t->simbolo_entrenador == symbol_expected);
	}
	entrenador = list_find(entrenadores_conectados, (void *) find_trainer);

	return entrenador;
}

t_pokenest * buscar_pokenest_por_id(char id) {

	t_pokenest * pokenest = NULL;

	bool find_pokenest(t_pokenest * pkn) {
		return (pkn->identificador == id);
	}
	pokenest = list_find(lista_de_pokenests, (void *) find_pokenest);

	return pokenest;
}

t_pokenest * buscar_pokenest_por_ubicacion(int x, int y) {
	t_pokenest * pknest = NULL;

	bool find_pokenest2(t_pokenest * pkn) {
		return ( (pkn->posicion->x == x)&&(pkn->posicion->y == y) );
	}
	pknest = list_find(lista_de_pokenests, (void *) find_pokenest2);

	return pknest;
}

t_entrenador * buscar_entrenador_por_socket(int expected_fd) {
	t_entrenador * entrenador = NULL;

	bool find_trainer2(t_entrenador * s_t) {
		return (s_t->socket == expected_fd);
	}
	entrenador = list_find(entrenadores_conectados, (void *) find_trainer2);

	return entrenador;
}

ITEM_NIVEL * buscar_item_por_id(char id) {

	ITEM_NIVEL * item;

	bool find_item(ITEM_NIVEL * i) {
		return (i->id == id);
	}
	item = list_find(items_mapa, (void *) find_item);

	return item;
}

//**********************************************************************************************

void destruir_metadata() {
	free(metadata->ip);
	free(metadata->medallaArchivo);
	free(metadata->planificador);
	free(metadata);
}

void ordenar_pokemons(t_list * pokemons) {

	int min, max;

	bool sorteame_estaaaaaa(t_pkm * shit_min, t_pkm * shit_max) {

		min = the_number_of_the_beast(shit_min);
		max = the_number_of_the_beast(shit_max);

		return (min <= max);
	}

	list_sort(pokemons, (void *) sorteame_estaaaaaa);

}

int liberar_pokemons(t_entrenador * e) {

	void free_pkm(t_pkm * p) {
		p->capturado = false;
		//Interfaz grafica
		//incrementar_recurso(p->id_pokenest);
	}
	list_iterate(e->pokemonesCapturados, (void *) free_pkm);

	list_clean(e->pokemonesCapturados);

	return EXIT_SUCCESS;
}

int incrementar_recurso(char id_pokenest) {

	pthread_mutex_lock(&mutex_gui);

	ITEM_NIVEL * item = buscar_item_por_id(id_pokenest);

	if (item == NULL)
		return EXIT_FAILURE;

	item->quantity++;

	pthread_mutex_unlock(&mutex_gui);

	return EXIT_SUCCESS;
}

void sacar_de_listos(t_entrenador * entrenador) {

	pthread_mutex_lock(&mutex_cola_listos);

	int listos = list_size(cola_de_listos);
	int i;
	for (i = 0; i < listos; i++) {
		t_entrenador * e = list_get(cola_de_listos, i);

		if (e->simbolo_entrenador == entrenador->simbolo_entrenador) {
			list_remove(cola_de_listos, i);
			break;
		}
	}

	pthread_mutex_unlock(&mutex_cola_listos);
}

void sacar_de_conectados(t_entrenador * entrenador) {

	pthread_mutex_lock(&(mutex_entrenadores));

	int totales = list_size(entrenadores_conectados);
	int i;

	for (i = 0; i < totales; i++) {
		t_entrenador * e = list_get(entrenadores_conectados, i);

		if (e->simbolo_entrenador == entrenador->simbolo_entrenador) {

			if (e == entrenador_corriendo) {
				keep_running = false;
				entrenador_corriendo = NULL;
				quantum_actual = 0;
			}

			list_remove(entrenadores_conectados, i);
			break;
		}
	}

	pthread_mutex_unlock(&(mutex_entrenadores));
}

void sacar_de_bloqueados(t_entrenador * e) {

	pthread_mutex_lock(&mutex_cola_bloqueados);

	int i;
	int n = list_size(cola_de_bloqueados);
	t_bloqueado * b;
	bool encontro = false;

	for (i = 0; i < n; i++) {
		b = list_get(cola_de_bloqueados, i);

		if (b->entrenador->simbolo_entrenador == e->simbolo_entrenador) {
			encontro = true;
			break;
		}
	}

	if (encontro)
		list_remove(cola_de_bloqueados, i);

	pthread_mutex_unlock(&mutex_cola_bloqueados);
}

t_pkm * obtener_primer_no_capturado(t_pokenest * pokenest) {
	t_pkm * pkm_capt = NULL;
	int pokemons = list_size(pokenest->pokemones);
	int i;

	for(i = 0; i < pokemons; i++) {
		t_pkm * p = list_get(pokenest->pokemones, i);

		if ( !(p->capturado) ) {
			pkm_capt = p;
			break;
		}
	}

	return pkm_capt;
}

int enviar_ruta_pkm(char * ruta, int socket) {

	int ruta_length = string_length(ruta);

	void * buff = malloc(ruta_length);
	memset(buff, 0, ruta_length);
	memcpy(buff, ruta, ruta_length);

	if (enviar_header(_CAPTURAR_PKM, ruta_length, socket) < 0)
		return -1;

	if (send(socket, buff, ruta_length, 0) < 0)
		return -1;

	free(buff);
	free(ruta);

	return EXIT_SUCCESS;
}

int agregar_a_cola(t_entrenador * entrenador, t_list * cola, pthread_mutex_t mutex) {

	int result = 0;

	pthread_mutex_lock(&mutex);
	bool ya_esta_en_cola = false;

	int i;
	for (i = 0; i < cola->elements_count; i++) {
		t_entrenador * e = list_get(cola, i);

		if (e->simbolo_entrenador == entrenador->simbolo_entrenador) {
			ya_esta_en_cola = true;
			break;
		}
	}

	if (!ya_esta_en_cola)
		list_add(cola, entrenador);
	else
		result = -1;
	pthread_mutex_unlock(&mutex);

	return result;
}

t_entrenador * pop_entrenador() {

	pthread_mutex_lock(&mutex_cola_listos);
	t_entrenador * e = list_remove(cola_de_listos, 0);
	pthread_mutex_unlock(&mutex_cola_listos);

	return e;
}

t_list * snapshot_list(t_list * source_list) {

	bool copy_this(void * elem) {
		return true;
	}
	t_list * copy = list_filter(source_list, (void *) copy_this);

	return copy;
}

int the_number_of_the_beast(t_pkm * beast) {

	int length_name = string_length(beast->nombre);
	int length_arch = string_length(beast->nombreArchivo);

	int i;
	char * num_on_string = NULL;

	for(i = length_name; i < length_arch; i++) {
		char c = beast->nombreArchivo[i];

		if (c == '.')
			break;
	}

	int length_number = i - length_name;

	num_on_string = malloc(1 + (length_number * sizeof(char)));
	num_on_string[length_number + 1] = '\0';
	memcpy(num_on_string, beast->nombreArchivo + length_name, length_number);

	int THE_number = atoi(num_on_string);

	free(num_on_string);

	return THE_number;
}
