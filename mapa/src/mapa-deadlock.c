/*
 * mapa-deadlock.c
 *
 *  Created on: 22/10/2016
 *      Author: utnso
 */

#include "mapa.h"
#include "mapa-deadlock.h"

void run_deadlock_thread() {

	factory = create_pkmn_factory();
	int retardoDL;

	while(!finalizacionDelPrograma) {
		pthread_mutex_lock(&mutex_metadata);
		retardoDL = metadata->tiempoChequeoDeadlock;
		pthread_mutex_unlock(&mutex_metadata);
		usleep(retardoDL);

		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "Se activa el algoritmo de deteccion de DEADLOCK");
		pthread_mutex_unlock(&mutex_log);

//		pthread_mutex_lock(&mutex_starvation);
		pthread_mutex_lock(&mutex_global);
		verificar_desconexion_por_starvation();
//		pthread_mutex_unlock(&mutex_starvation);
		pthread_mutex_unlock(&mutex_global);

		if (run_deadlock_algorithm() == -1) {
			pthread_mutex_lock(&mutex_log);
			log_error(logger, "[DEADLOCK]: El algoritmo de deteccion de deadlock devolvio un resultado de error.");
			pthread_mutex_unlock(&mutex_log);
			break;
		}
	}
}

void verificar_desconexion_por_starvation() {
	int i;
	t_header * header = NULL;

//	pthread_mutex_lock(&mutex_cola_bloqueados);
	for (i = 0; i < cola_de_bloqueados->elements_count; i++) {
		t_bloqueado * b = list_get(cola_de_bloqueados, i);

		if (b == NULL)
			continue;

//		pthread_mutex_lock(&(b->entrenador->mutex_entrenador));

		struct timeval tv;

		tv.tv_sec = 1;  /* 1 Secs Timeout */
		tv.tv_usec = 0;  // Not init'ing this can cause strange errors

		setsockopt(b->entrenador->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

		header = recibir_header(b->entrenador->socket);

		int optval = 1;
		setsockopt(b->entrenador->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

		if (header == NULL) {
//			pthread_mutex_unlock(&(b->entrenador->mutex_entrenador));
			continue;
		}

		if (header->identificador == _DESCONEXION) {
			pthread_mutex_lock(&mutex_log);
			log_trace(logger, "El entrenador %c finalizo por algun motivo, o se mato el proceso por estar en inanicion.", b->entrenador->simbolo_entrenador);
			pthread_mutex_unlock(&mutex_log);
//			pthread_mutex_unlock(&(b->entrenador->mutex_entrenador));
//			sacar_de_bloqueados(b->entrenador);
			desconexion_entrenador(b->entrenador, -3);
			i--;
//			free(b);
//			b = NULL;
		} else {
//			pthread_mutex_unlock(&(b->entrenador->mutex_entrenador));
		}

		free(header);
		header = NULL;
	}

//	pthread_mutex_unlock(&mutex_cola_bloqueados);
}

int run_deadlock_algorithm() {

	if (snapshot_del_sistema() == -1)
		return 0;

	int pos = 0;
	int * temp_disp = vector_copy(disponibles, temp_pokenests->elements_count);

	//marcar aquellos que no tienen ningun pokemon asignado
	marcar_sin_pkms(marcados);
	int veces_que_itera = cantidad_no_marcados(temp_entrenadores->elements_count, marcados);

	//***************** BEGIN ALGORITHM ****************************************

	while (!(todos_marcados(temp_entrenadores->elements_count, marcados) || (veces_que_itera <= 0))) {

		if (marcados[pos] == 1) {
			pos++;
			if (pos >= temp_entrenadores->elements_count)
				pos = 0;

			continue;
		}

		veces_que_itera--;

		if (puede_asignar(temp_pokenests->elements_count, solicitudes->data[pos], temp_disp)) {
			//le sumo la asignacion al temporal disponible
			int j;
			for (j = 0; j < temp_pokenests->elements_count; j++) {
				temp_disp[j] += asignados->data[pos][j];
			}
			//marco al entrenador
			marcados[pos] = 1;

			veces_que_itera = cantidad_no_marcados(temp_entrenadores->elements_count, marcados);
		}

		pos++;

		if (pos >= temp_entrenadores->elements_count)
			pos = 0;

	}
	//***************** END ALGORITHM ****************************************

	int cantidad_en_DL = cantidad_no_marcados(temp_entrenadores->elements_count, marcados);
	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "[DEADLOCK] fin del algoritmo con %d entrenadores en dl.", cantidad_en_DL);
	pthread_mutex_unlock(&mutex_log);
	t_entrenador * loser = NULL;

	if (cantidad_en_DL > 1) {
		if (metadata->batalla) {
			pthread_mutex_lock(&mutex_global);
			loser = let_the_battle_begins();

			if (loser == NULL) {
				pthread_mutex_lock(&mutex_log);
				log_error(logger, "El algoritmo de DL finalizo de forma abrupta.");
				pthread_mutex_unlock(&mutex_log);
				return EXIT_SUCCESS;
			}

			pthread_mutex_lock(&mutex_log);
			log_trace(logger, "[DEADLOCK] El entrenador %c(%s) ha perdido la batalla.", loser->simbolo_entrenador, loser->nombre_entrenador);
			pthread_mutex_unlock(&mutex_log);
			desconexion_entrenador(loser, 1);
			pthread_mutex_unlock(&mutex_global);
		}
	}

	destroy_vector(temp_disp);
	release_all();

	return EXIT_SUCCESS;
}

void release_all() {
	destroy_matriz(asignados);
	destroy_matriz(solicitudes);
	destroy_vector(disponibles);
	destroy_vector(marcados);
//	free(temp_entrenadores);
	list_destroy(temp_pokenests);
	list_destroy(temp_entrenadores);
//	free(temp_pokenests);
//	list_destroy_and_destroy_elements(temp_entrenadores, (void *) entrenador_destroyer);
//	list_destroy_and_destroy_elements(temp_pokenests, (void *) pokenest_destroyer);
}

void destroy_matriz(t_matriz * matriz) {
	int f;

	for (f = 0; f < temp_entrenadores->elements_count; f++) {
		free(matriz->data[f]);
	}

	free(matriz->data);
	free(matriz);
}

void destroy_vector(int * vector) {
	free(vector);
}

int snapshot_del_sistema() {

	pthread_mutex_lock(&mutex_global);
//	pthread_mutex_lock(&mutex_planificador_turno);

	if (entrenadores_conectados->elements_count <= 1) {
//		pthread_mutex_unlock(&mutex_planificador_turno);
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "No se procede a ejecutar el algoritmo ya que se encontraron %d entrenadores en el sistema.", entrenadores_conectados->elements_count);
		pthread_mutex_unlock(&mutex_log);
		pthread_mutex_unlock(&mutex_global);
		return -1;
	}

//	pthread_mutex_lock(&mutex_pokenests);
	temp_pokenests = snapshot_list(lista_de_pokenests);
	disponibles = crear_vector_Disponibles();
//	pthread_mutex_unlock(&mutex_pokenests);

//	pthread_mutex_lock(&mutex_entrenadores);
	temp_entrenadores = snapshot_list(entrenadores_conectados);
	asignados = crear_matriz_Asignados();
		pthread_mutex_lock(&mutex_cola_bloqueados);
		solicitudes = crear_matriz_Solicitudes();
		pthread_mutex_unlock(&mutex_cola_bloqueados);
//	pthread_mutex_unlock(&mutex_entrenadores);

	pthread_mutex_lock(&mutex_log);
	imprimir_pokenests_en_log();
	imprimir_vector_en_log(disponibles, "Vector de disponibles", temp_pokenests->elements_count);
	imprimir_entrenadores_en_log();
	imprimir_matriz_en_log(asignados, "Matriz Asignados");
	imprimir_matriz_en_log(solicitudes, "Matriz Solicitudes");
	pthread_mutex_unlock(&mutex_log);

//	pthread_mutex_unlock(&mutex_planificador_turno);
	pthread_mutex_unlock(&mutex_global);

	marcados = crear_vector(temp_entrenadores->elements_count);

	return 0;
}

t_matriz * crear_matriz(int filas, int columnas) {
	t_matriz * matriz = malloc(sizeof(t_matriz));
	matriz->filas = filas;
	matriz->columnas = columnas;
	int f, c;

	matriz->data = (int **) malloc(filas * sizeof(int *));
	for (f = 0; f < filas; f++) {
		matriz->data[f] = (int *) malloc(columnas * sizeof(int));
	}

	//luego la inicializo
	for (f = 0; f < filas; f++) {
		for (c = 0; c < columnas; c++) {
			matriz->data[f][c] = 0;
		}
	}

	return matriz;
}

void imprimir_matriz_en_log(t_matriz * matriz, char * nombre_matriz) {
	char * mat = string_new();
	string_append_with_format(&mat, "\n***_%s_***\n", nombre_matriz);

	int i, j;

	for (i = 0; i < matriz->filas; i++) {
		string_append(&mat, "| ");
		for (j = 0; j < matriz->columnas; j++) {
			string_append_with_format(&mat, " %d ", matriz->data[i][j]);
		}
		string_append(&mat, " |\n");
	}
	string_append(&mat, " \0");

	log_trace(logger, mat);

	free(mat);
}

void imprimir_vector_en_log(int * vector, char * nombre_vector, int rows) {
	char * vec = string_new();
	string_append_with_format(&vec, "\n***_%s_***\n", nombre_vector);

	int i;

	string_append(&vec, "[ ");
	for (i = 0; i < rows; i++) {
		string_append_with_format(&vec, " %d ", vector[i]);
	}
	string_append(&vec, " ]\0");

	log_trace(logger, vec);

	free(vec);
}

void imprimir_pokenests_en_log() {
	char * list = string_new();
	string_append(&(list), "\n***_POKENESTS:_***\n");

	int i;
	string_append(&list, "[ ");
	for (i = 0; i < temp_pokenests->elements_count; i++) {
		t_pokenest * p = list_get(temp_pokenests, i);
		string_append_with_format(&list, " %c ", p->identificador);
	}
	string_append(&list, " ]\0");

	log_trace(logger, list);

	free(list);
}

void imprimir_entrenadores_en_log() {
	char * list = string_new();
	string_append(&(list), "\n***_ENTRENADORES:_***\n");

	int i;
	string_append(&list, "[ ");
	for (i = 0; i < temp_entrenadores->elements_count; i++) {
		t_entrenador * e = list_get(temp_entrenadores, i);
		string_append_with_format(&list, " %c ", e->simbolo_entrenador);
	}
	string_append(&list, " ]\0");

	log_trace(logger, list);

	free(list);
}

int * vector_copy(int * vec_src, int elems) {
	int * vec_cpy = crear_vector(elems);

	int i;
	for(i = 0; i < elems; i++) {
		vec_cpy[i] = vec_src[i];
	}

	return vec_cpy;
}

t_matriz * crear_matriz_Asignados() {

	int i, j, k;
	int pkms = 0;

	t_matriz * m_asignados = crear_matriz(temp_entrenadores->elements_count, temp_pokenests->elements_count);

	for (i = 0; i < temp_entrenadores->elements_count; i++) {

		t_entrenador * e = list_get(temp_entrenadores, i);

		pkms = list_size(e->pokemonesCapturados);

		for (j = 0; j < pkms; j++) {
			t_pkm * p = list_get(e->pokemonesCapturados, j);

			k = indice_en_lista_pokenests(p->id_pokenest);

			m_asignados->data[i][k] += 1;
		}

	}

	return m_asignados;
}

int * crear_vector(int elems) {
	int array_size = elems * sizeof(int);

	int * vec = malloc(array_size);
	memset(vec, 0, array_size);

	return vec;
}

int * crear_vector_Disponibles() {

	disponibles = crear_vector(temp_pokenests->elements_count);

	int i,j;
	int count = 0;

	for (i = 0; i < temp_pokenests->elements_count; i++) {
		t_pokenest * pknst = list_get(temp_pokenests, i);

		for (j = 0; j < pknst->pokemones->elements_count; j++) {
			t_pkm * pkm = list_get(pknst->pokemones, j);

			if (pkm->capturado == false)
				count++;
		}

		disponibles[i] = count;
		count = 0;
	}

	return disponibles;
}

t_matriz * crear_matriz_Solicitudes() {
	t_matriz * m_solicitudes = crear_matriz(temp_entrenadores->elements_count, temp_pokenests->elements_count);
	int i, f, c;

	for(i = 0; i < cola_de_bloqueados->elements_count; i++) {
		t_bloqueado * e_b = list_get(cola_de_bloqueados, i);

		f = indice_en_lista_entrenadores(e_b->entrenador->simbolo_entrenador);
		c = indice_en_lista_pokenests(e_b->pokenest->identificador);

		m_solicitudes->data[f][c] += 1;
	}

	return m_solicitudes;
}

int indice_en_lista_entrenadores(char simbolo_entrenador) {
	int i;

	for(i = 0; i < temp_entrenadores->elements_count; i++) {
		t_entrenador * e = list_get(temp_entrenadores, i);

		if (e->simbolo_entrenador == simbolo_entrenador)
			break;
	}

	return i;
}

int indice_en_lista_pokenests(char id) {
	int i;
	for (i = 0; i < temp_pokenests->elements_count; i++) {
		t_pokenest * pkn = list_get(temp_pokenests, i);

		if ( pkn->identificador == id )
			break;
	}

	return i;
}

void marcar_sin_pkms() {
	int i;

	for(i = 0; i < temp_entrenadores->elements_count; i++) {
		t_entrenador * e = list_get(temp_entrenadores, i);
		if(list_is_empty(e->pokemonesCapturados))
			marcados[i] = 1;
	}
}

int cantidad_no_marcados(int x, int * vec) {
	int i;
	int n = 0;

	for (i = 0; i < x; i++) {
		if (vec[i] == 0)
			n++;
	}

	return n;
}

bool todos_marcados(int x, int * vec) {
	int i;
	bool todos = true;

	for (i = 0; i < x; i++) {
		if (vec[i] == 0) {
			todos = false;
			break;
		}
	}

	return todos;
}

bool puede_asignar(int x, int * sol, int * disp) {
	bool puede = true;
	int i;
	int s, d;

	for (i = 0; i < x; ++i) {

		s= sol[i];
		d = disp[i];

		if ((d - s) < 0) {
			puede = false;
			break;
		}
	}

	return puede;
}

t_list * obtener_los_dls() {
	t_list * entrenadores_en_DL = list_create();
	int m;
	for (m = 0; m < temp_entrenadores->elements_count; m++) {
		if (marcados[m] == 0) {
			t_entrenador * e = list_get(temp_entrenadores, m);

			if (e->socket != -1)
				list_add(entrenadores_en_DL, e);
		}
	}

	return entrenadores_en_DL;
}

void loguear_entrenadores_en_dl(t_list * lista) {

	char * mssg = string_new();
	int i;

	for (i = 0; i < lista->elements_count; i++) {
		t_entrenador * e = list_get(lista, i);

		string_append_with_format(&mssg, " %c(%s) ", e->simbolo_entrenador, e->nombre_entrenador);
	}

	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "LISTA DEADLOCK: [%s]", mssg);
	pthread_mutex_unlock(&mutex_log);

	free(mssg);
}

t_entrenador * let_the_battle_begins() {

	t_list * dls = obtener_los_dls();
	loguear_entrenadores_en_dl(dls);

	t_entrenador * entrenador_que_perdio = list_get(dls, 0);
	t_pokemon * pkm_del_que_perdio = obtener_el_mas_poronga(entrenador_que_perdio);

	if (pkm_del_que_perdio == NULL) {
		free(dls);
		return NULL;
	}

	t_pokemon * loser = NULL;
	t_pokemon * opponent = NULL;

	int i;
	for (i = 1; i < dls->elements_count; i++) {
		t_entrenador * entrenador_oponente = list_get(dls, i);
		opponent = obtener_el_mas_poronga(entrenador_oponente);

		if (opponent == NULL) {
			//TODO VER BIEN ESTO!!!!!!!!
			//HANDLEARLO DE LA FORMA MAS FELIZ POSIBLE
			free(dls);
			return NULL;
		}

		loser = pkmn_battle(pkm_del_que_perdio, opponent);

		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "Batalla pkm: %s[%d](%s) vs %s[%d](%s)",
				pkm_del_que_perdio->species, pkm_del_que_perdio->level, entrenador_que_perdio->nombre_entrenador,
				opponent->species, opponent->level, entrenador_oponente->nombre_entrenador);
		log_trace(logger, "Perdedor: %s [%d]", loser->species, loser->level);
		pthread_mutex_unlock(&mutex_log);

		if (pkm_del_que_perdio == loser){
			//sigue peleando
			free(opponent->species);
			free(opponent);
		} else { //opponent == loser
			free(pkm_del_que_perdio->species);
			free(pkm_del_que_perdio);
			pkm_del_que_perdio = opponent;
			entrenador_que_perdio = entrenador_oponente;
		}

		opponent = NULL;
	}

	if (opponent != NULL) {
		free(opponent->species);
		free(opponent);
		opponent = NULL;
	}

	if (pkm_del_que_perdio != NULL) {
		free(pkm_del_que_perdio->species);
		free(pkm_del_que_perdio);
		pkm_del_que_perdio = NULL;
	}


	void * _on_error(char simb) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger,
				"Error al enviar el resultado de la batalla al entrenador %c .",
				simb);
		pthread_mutex_unlock(&mutex_log);
		return NULL;
	}

	for (i = 0; i < dls->elements_count; i++) {
		t_entrenador * e = list_get(dls, i);
		e->deadlocksInvolucrados++;

		if (e->simbolo_entrenador == entrenador_que_perdio->simbolo_entrenador) {
			//envio que perdio
			//como perdio le tengo que mandar los datos:
			//deadlocks involucrados / tiempo bloqueado / tiempo de aventura
			if (avisar_que_perdio_la_batalla(entrenador_que_perdio) == -1)
				return _on_error(e->simbolo_entrenador);
		} else {
			//envio que gano
			if (enviar_header(_RESULTADO_BATALLA, 1, e->socket) == -1)
				return _on_error(e->simbolo_entrenador);
		}
	}

	list_destroy(dls);

	return entrenador_que_perdio;
}

int avisar_que_perdio_la_batalla(t_entrenador * entrenador) {
	if (enviar_header(_RESULTADO_BATALLA, 0, entrenador->socket) == -1)
		return -1;

	time_t tiempo_desbloqueo;
	time(&tiempo_desbloqueo);

	entrenador->bloqueado = false;
	entrenador->tiempoBloqueado += difftime(tiempo_desbloqueo, entrenador->momentoBloqueado);
	entrenador->momentoBloqueado = 0;

	if (enviar_datos_finales_entrenador(entrenador) == -1)
		return -1;

	return 0;
}

t_pokemon * obtener_el_mas_poronga(t_entrenador * entrenador) {

	t_pokemon * _on_error() {
		desconexion_entrenador(entrenador, 0);
		return NULL;
	}

	//Le avisa al entrenador que hay batalla
	if ( enviar_header(_BATALLA, 0, entrenador->socket) < 0 ) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "No se pudo enviar el header _BATALLA a entrenador %c .", entrenador->simbolo_entrenador);
		pthread_mutex_unlock(&mutex_log);
		return _on_error();
	}

	t_header* headerpkmMasFuerte = recibir_header(entrenador->socket);

	if (headerpkmMasFuerte == NULL) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger,
				"Header NULL del pokemon para la batalla. Entrenador: %c",
				entrenador->simbolo_entrenador);
		pthread_mutex_unlock(&mutex_log);
		return _on_error();
	}

	if(headerpkmMasFuerte->identificador != _PKM_MAS_FUERTE){
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Id_header incorrecto. Se esperaba: _PKM_MAS_FUERTE");
		pthread_mutex_unlock(&mutex_log);
		free(headerpkmMasFuerte);
		return _on_error();
	}

	t_pkm * pokemonMasFuerte = recibirYDeserializarPokemon(entrenador->socket,
			headerpkmMasFuerte->tamanio);

	if (pokemonMasFuerte == NULL) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger,
				"Error en el recv del pokemon mas fuerte. Entrenador: %c",
				entrenador->simbolo_entrenador);
		pthread_mutex_unlock(&mutex_log);
		free(headerpkmMasFuerte);
		return _on_error();
	}

	char * nombre = string_duplicate(pokemonMasFuerte->nombre);

	t_pokemon * pkm_to_return = create_pokemon(factory, nombre, pokemonMasFuerte->nivel);

	free(pokemonMasFuerte->nombre);
	free(pokemonMasFuerte->nombreArchivo);
	free(pokemonMasFuerte);
	free(headerpkmMasFuerte);

	return pkm_to_return;
}

t_entrenador * buscar_entrenador_del_pkm(t_pokemon * pkm, t_list * lista) {
	int i;
	t_entrenador * e = NULL;

	bool func(t_pkm * p) {
		return ((string_equals_ignore_case(p->nombre, pkm->species))
				&& (p->nivel == pkm->level));
	}

	for (i = 0; i < lista->elements_count; i++) {
		e = list_get(lista, i);



		if (list_any_satisfy(e->pokemonesCapturados, (void *) func))
			break;
	}

	return e;
}
