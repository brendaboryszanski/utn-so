#include "operaciones.h"

int validarArchivo(int emisor){
	char* rutaArchivo = deserializarString(emisor);
	log_info(logger, "Validando archivo en ruta: %s", rutaArchivo);
	return true;
	//TODO validar el archivo posta
}
void crearArchivo(int emisor){
	char* rutaArchivo = deserializarString(emisor);
	log_info(logger, "Creando archivo en ruta: %s", rutaArchivo);
	//TODO crear el archivo posta
}

void guardarDatos(int emisor){
	char* rutaArchivo = deserializarString(emisor);
	char* offset = deserializarString(emisor);
	int tamanioALeer = deserializarInt(emisor);
	char* datos = deserializarString(emisor);
	log_info("Guardando datos: %s en archivo: %s", datos, rutaArchivo);
	//TODO guardar datos posta y hacer free de los strings
}
char* obtenerDatos(int emisor){
	char* rutaArchivo = deserializarString(emisor);
	int offset = deserializarInt(emisor);
	int tamanioALeer = deserializarInt(emisor);
	log_info("Obteniendo %d bytes de archivo: %s", tamanioALeer, rutaArchivo);
	return "leyendo de archivo";
	//TODO leer archivos posta y hacer free de los strings
 }
