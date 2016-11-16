/*
 * servidorPokedex.h
 *
 *  Created on: 5/10/2016
 *      Author: utnso
 */

#include <comunicacion.h>
#include <osada.h>
#include <signal.h>
#include <pthread.h>


#ifndef SERVIDORPOKEDEX_H_
#define SERVIDORPOKEDEX_H_

#define PUERTO "4969"
#define BACKLOG 15	// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo


#define HANDSHAKE 777
#define ERROR -1

#define PEDIDO_GETATTR 12
#define PEDIDO_READDIR 13
#define PEDIDO_TRUNCATE 14
#define PEDIDO_READ 15
#define PEDIDO_WRITE 16
#define PEDIDO_UNLINK 17
#define PEDIDO_MKDIR 18
#define PEDIDO_RMDIR 19
#define PEDIDO_RENAME 20
#define PEDIDO_CREATE 21

#define RESPUESTA_GETATTR 22
#define RESPUESTA_READDIR 23
#define RESPUESTA_TRUNCATE 24
#define RESPUESTA_READ 25
#define RESPUESTA_WRITE 26
#define RESPUESTA_UNLINK 27
#define RESPUESTA_MKDIR 28
#define RESPUESTA_RMDIR 29
#define RESPUESTA_RENAME 30
#define RESPUESTA_CREATE 31
#define ENOENTRY 32

#define PEDIDO_OPEN 33
#define RESPUESTA_OPEN 34
#define PEDIDO_RELEASE 35
#define RESPUESTA_RELEASE 36
#define PEDIDO_TRUNCATE_NEW_SIZE 37
#define PEDIDO_FLUSH 38
#define RESPUESTA_FLUSH 39

#define PEDIDO_UTIMENS 40
#define RESPUESTA_UTIMENS 47
#define RESPUESTA_ERROR 41

#define ERRDQUOT 42 //archivo 2049, no hay espacio en la tabla de archivos
#define ERRFBIG 43 //no hay bloques de datos disponibles
#define ERRNAMETOOLONG 44 //nombres de archivos con mas de 17 caracteres
#define PEDIDO_MKNOD 45
#define RESPUESTA_MKNOD 46

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define RESET "\x1B[0m"
#define NAR "\033[38;5;166m"//naranja
#define PINK "\033[38;5;168m"//rosa
#define VIO "\033[38;5;129m"//violeta
#define PINK2 "\033[38;5;207m"//violeta

#define BLOCK_SIZE	64

//Variables Globales
t_log logServidor;
int	listenningSocket;
t_bitarray* bitmap;
pthread_mutex_t mutex_comunicacion  = PTHREAD_MUTEX_INITIALIZER;

void atendercliente(int socket);
void* hiloComunicacion(void* arg);
void liberarRecursos(); //TODO
void printEncabezado();
void printTerminar();

void* procesarCrearEntradaTablaDeArchivos(char *path, int* codigo, int modo);

void* procesarPedidoCreate(char *path, int* codigo);
void* procesarPedidoGetatrr(char *path);
void* procesarPedidoFlush(char *path);
void* procesarPedidoMkdir(char *path, int* codigo);
void* procesarPedidoMknod(char *path, int* codigo);
void* procesarPedidoOpen(char* path);
void* procesarPedidoRead(void* buffer, uint32_t* tamanioBuffer);
void* procesarPedidoReaddir(char *path);
void* procesarPedidoRelease(char* path);
void* procesarPedidoRename(char *paths);
void* procesarPedidoRmdir(char *path);
void* procesarPedidoTruncate(off_t newSize, char* path);
void* procesarPedidoUnlink(char *path);
void* procesarPedidoUtimens(char *path);
void* procesarPedidoWrite(void *buffer);

void terminar();

#endif /* SERVIDORPOKEDEX_H_ */
