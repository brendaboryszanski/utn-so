#ifndef SEGMENTACIONPURA_H_
#define SEGMENTACIONPURA_H_

#include "FM9.h"

typedef struct{
	int id;
	int limite;
	int base;
	int nombreArchivo;
}ElementoTablaSegPura;

int idSegmento;
t_list* tablaDeSegmentos; //Es una lista de ElementoTablaSegPura

void inicializarSegPura();
int guardarDatosSegPura(char* datos, char* nombreArchivo);
respuestaDeObtencionDeMemoria* obtenerDatosSegPura(t_list* idsSegmentos);
respuestaDeObtencionDeMemoria* obtenerLineaSegPura(char* nombreArchivo, int numeroLinea);
#endif /* SEGMENTACIONPURA_H_ */
