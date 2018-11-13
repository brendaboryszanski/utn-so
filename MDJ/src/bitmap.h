#ifndef BITMAP_H_
#define BITMAP_H_


#include "comandos.h"
#include "operacionesArchivos.h"

int ocuparBloqueEnBitmap(int bloqueAOcupar);
int liberarBloqueEnBitmap(int bloqueALiberar);

char* rutaBitmap;

extern t_log* logger;
extern char* PUNTO_MONTAJE_METADATA;
#endif /*BITMAP_H_*/
