/*
 * entrenador.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared_configs.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <signal.h>
#include <sys/socket.h>

#define TOTAL_ARGS 3

#define _ID_HANDSHAKE 1
#define _TURNO_CONCEDIDO 2
#define _UBICACION_POKENEST 3
#define _MOVER_XY 4
#define _CAPTURAR_PKM 5
#define _OBJETIVO_CUMPLIDO 6
#define _QUEDAN_OBJETIVOS 7
#define _BATALLA 8
#define _PKM_MAS_FUERTE 9
#define _RESULTADO_BATALLA 10
#define _DATOS_FINALES 11


typedef struct {
	char * nombre;
	char simbolo;
	t_queue * viaje;  //lista de t_mapa  (hoja de viaje + objetivos)
	int vidas;
	int reintentos;
} t_metadata_entrenador;

typedef struct {
	char * nombre_mapa;
	int puerto;
	char * ip;
	int socket;
	t_queue * objetivos;
} t_mapa;

typedef struct {
	int x;
	int y;
} t_posicion;

typedef struct {
	char* nombreArchivo;
	char* nombre;
	int nivel;
} t_pokemon;


char* metadata_path;
char* nombreEntrenador;
int socket_entrenador;
t_metadata_entrenador * metadata;
t_posicion * ubicacionActual;
t_posicion * ubicacionProximaPokenest;
t_list * pokemonesCapturados;    		//t_pokemon
t_mapa * mapaActual;
int cantidadDeMuertes;
t_pokemon * pokemonMasFuerte;
bool pokenestLocalizada;
bool finDelJuego;
float tiempoBloqueado;
int deadlocksInvolucrados;
float tiempoDeJuego;




//------------------Funciones

t_mapa * crear_mapa(char * nombre_mapa);
t_metadata_entrenador * crear_metadata();
int leer_metadata_entrenador(char * metada_path);
void imprimir_metadata();
int conectarse_a_un_mapa(int puerto, char * ip);
int enviar_datos_a_mapa(int socket, char simbolo, char * nombre);

void inicializarEntrenador();
void inicializarSinmuertesNiReintentos();
void cargarMetadata();
void conectarseConSiguienteMapa();
void solicitarUbicacionDelProximoPokenest();
void enviarSolicitudUbicacionPokenest(char id_pokemon);
void avanzarHaciaElPokenest();
void atraparPokemon();
void verificarSiQuedanObjetivosEnMapa();
void verificarNivelPokemon(t_pokemon * pokemon);
void esperarTurno();
void realizarAccion();
bool estoyEnPokenest();
void imprimirLogro();
bool batallaPokemon();
void muerteEntrenador();
void desconectarseDeMapa();
void finalizarEntrenador();
void destruirHojaDeViaje();
void rutina(int signal);
void enviarUbicacionAMapa();


#endif /* ENTRENADOR_H_ */
