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

	while(true) {
		usleep(metadata->tiempoChequeoDeadlock);

		if (run_deadlock_algorithm() == -1) {
			pthread_mutex_lock(&mutex_log);
			log_trace(logger, "[ERROR]: El algoritmo de deteccion de deadlock devolvio un resultado de error.");
			pthread_mutex_unlock(&mutex_log);
			break;
		}
	}
}

int run_deadlock_algorithm() {

	snapshot_del_sistema();

	int pos = 0;
	//AUN NO SE SI LO VOY A NECESITAR
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

	if (cantidad_en_DL > 0) {
		if (metadata->batalla) {
			loser = let_the_battle_begins();
			//desconexion_entrenador(loser, -1);
		}
	}

	free(temp_disp);

	return EXIT_SUCCESS;
}

void snapshot_del_sistema() {

	temp_entrenadores = list_create();
	temp_pokenests = list_create();

	pthread_mutex_lock(&mutex_pokenests);
	temp_pokenests = snapshot_list(lista_de_pokenests);
	disponibles = crear_vector_Disponibles();
	pthread_mutex_unlock(&mutex_pokenests);

	pthread_mutex_lock(&mutex_entrenadores);
	temp_entrenadores = snapshot_list(entrenadores_conectados);
	asignados = crear_matriz_Asignados();

		pthread_mutex_lock(&mutex_cola_bloqueados);
		solicitudes = crear_matriz_Solicitudes();
		pthread_mutex_unlock(&mutex_cola_bloqueados);

	pthread_mutex_unlock(&mutex_entrenadores);

	marcados = crear_vector(temp_entrenadores->elements_count);

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

	log_trace(logger, mat);
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

	int i = 0;

	void iterate_pokenests(t_pokenest * pknst) {
		int availables = 0;
		void iterate_available_pkm(t_pkm * pkm) {
			if(pkm->capturado == false)
				availables++;
		}
		list_iterate(pknst->pokemones, (void *) iterate_available_pkm);

		disponibles[i] = availables;
	}
	list_iterate(temp_pokenests, (void *) iterate_pokenests);

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

			list_add(entrenadores_en_DL, e);
		}
	}

	return entrenadores_en_DL;
}

t_entrenador * let_the_battle_begins() {

	t_list * dls = obtener_los_dls();

	// En caso de que solo sea uno no tiene sentido armar bardo
	if (dls->elements_count == 1)
		return list_get(dls, 0);

	t_pokemon * winner = obtener_el_mas_poronga((t_entrenador *) list_get(dls, 0));
	t_pokemon * loser = NULL;

	int i;
	for (i = 1; i < dls->elements_count; i++) {
		t_pokemon * opponent = obtener_el_mas_poronga((t_entrenador *) list_get(dls, i));

		loser = pkmn_battle(winner, opponent);

		if (loser == winner)
			winner = opponent;
	}

	t_entrenador * el_entrenador_que_PERDIO = buscar_entrenador_del_pkm(loser, dls);

	free(dls);

	return el_entrenador_que_PERDIO;
}

t_pokemon * obtener_el_mas_poronga(t_entrenador * entrenador) {
	t_pkm * p = list_get(entrenador->pokemonesCapturados, 0);

	//TODO PEDIRLE AL ENTRENADOR EL QUE LA TIENE MAS LARGA

	t_pokemon * pkm = create_pokemon(factory, p->nombre, p->nivel);

	return pkm;
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