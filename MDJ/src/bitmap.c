#include "bitmap.h"

int cantidadBloquesBitmapEnBytes(){
	int total = CANTIDAD_BLOQUES/8;
	if(CANTIDAD_BLOQUES % 8 > 0){
		total++;
	}
	return total;
}

t_bitarray* crearBitarray(){
	char* byteArray = asignarMemoria(cantidadBloquesBitmapEnBytes());

	FILE* archivoBitMap = fopen(rutaBitmap, "rb");
	fread(byteArray, cantidadBloquesBitmapEnBytes(), 1, archivoBitMap);

	return bitarray_create_with_mode(byteArray, cantidadBloquesBitmapEnBytes(), MSB_FIRST);
}

void persistirBitMap(){
	char* bitArray = asignarMemoria(0);
	for(int i = 0; i < CANTIDAD_BLOQUES; i++){
		concatenarATexto(&bitArray, intToString(bitarray_test_bit(bitarray, i)));
	}

	FILE* archivoBitMap = fopen(rutaBitmap, "wb");  // w for write, b for binary

	fwrite(bitArray, bitarray_get_max_bit(bitArray), 1, archivoBitMap);

	fclose(archivoBitMap);
}

void initBitmap(){
	rutaBitmap = concatenar(PUNTO_MONTAJE_METADATA, "Bitmap.bin");
	bitarray = crearBitarray();
	printf("INIT- la cant de bloques %d \n", bitarray_get_max_bit(bitarray));
	printf("INIT- cant libres es %d \n", cantidadTotalDeBloquesLibres());
	for(int i = 0; i < bitarray_get_max_bit(bitarray); i++){
		printf("%d", bitarray_test_bit(bitarray, i));
	}
	printf("\n");
}

int cantidadTotalDeBloquesLibres(){
	int cantidadLibre = 0;
	for(int i = 0; i < bitarray_get_max_bit(bitarray); i++){
		if(bitarray_test_bit(bitarray, i) == 0){
			cantidadLibre++;
		}
	}
	return cantidadLibre;
}

int ocuparBloqueEnBitmap(int bloqueAOcupar){
	bitarray_set_bit(bitarray, bloqueAOcupar);
	//persistirBitMap();
}

int liberarBloqueEnBitmap(int bloqueALiberar){
	bitarray_clean_bit(bitarray, bloqueALiberar);
	//persistirBitMap();
}

int obtenerBloqueLibreBitmap(){
	int posicion;
	for(int i = 0; i < bitarray_get_max_bit(bitarray); i++){
		if(bitarray_test_bit(bitarray, i) == 0){
			posicion = i;
			break;
		}
	}
	return posicion;
}






