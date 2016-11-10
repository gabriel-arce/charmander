/*
 * servidorPokedex.h
 *
 *  Created on: 5/10/2016
 *      Author: utnso
 */

#ifndef SERVIDORPOKEDEX_H_
#define SERVIDORPOKEDEX_H_

#define PUERTO "4969"
#define BACKLOG 15	// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
//pasar a un .h los pedidos respuestas
#define HANDSHAKE 777

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

//funciones de disco
void asignarOffsets();
void descargar(uint32_t descriptorArchivo);
void escribirBloque(uint32_t bloque, char* buf);
void escribirArchivo(uint32_t posicion, char* buf);
int existePath(char* path, uint16_t** pos);
int existeDirectorio(unsigned char* token, uint16_t* padre, int* posicion);
void inicializarDisco();
void leerArchivo(uint32_t posicion, osada_file* buf);
void leerAsignacion(uint32_t posicion, osada_file* buf);
void leerBloque(uint32_t cantidadBloques, char* buf);
void leerDato(uint32_t posicion, osada_file* buf);
void leerHeader();
int mapearDisco(char* path);
void mostrarHeader(osada_header oheader);
void* readdir(char* path);
void leerTablaArchivos();
void leerTablaAsignaciones();
void leerTablaDatos();

char borrarArchivo(char* path);
char borrarDirectorio(char* path);
char crearArchivo(char* path, int modo);
char renombrarArchivo(char* paths);


//estas no se si funcionan
void* readFile(osada_file ofile, void *buffer);
void* readData(int cant_blocks, int* fat_values, void *buffer);
void* concatenate(void *buffer, int tamBuffer, void *tmpBuffer, int tamTmpBuffer, void* result);


//funciones de servidor-------------------------------------------------
void atendercliente(int socket);
void* hiloComunicacion(void* arg);
void printEncabezado();
void printTerminar();

void* procesarPedidoCreate(char *pedido);
void* procesarPedidoGetatrr(char *path);
void* procesarPedidoMkdir(char *path);
void* procesarPedidoRead(void *buffer);
void* procesarPedidoReaddir(char *path);
void* procesarPedidoRename(char *paths);
void* procesarPedidoRmdir(char *path);
void* procesarPedidoTruncate(char *path);
void* procesarPedidoUnlink(char *path);
void* procesarPedidoWrite(void *buffer);

void terminar();

#endif /* SERVIDORPOKEDEX_H_ */