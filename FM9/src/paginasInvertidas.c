#include "paginasInvertidas.h"

static void cargarTabla();
static int obtenerMarcoLibreInvertida();
static ElementoTablaInvertida* obtenerElementoPorIdDTB(int idDTB);
static ElementoTablaInvertida* obtenerElementoPorMarco(int marco);
static t_list* filtrarPorDTBYArchivo(int idDTB, char* nombreArchivo);
static int cantidadDeLineasArchivo(int idDTB, char* nombreArchivo);

void inicializarInvertida(t_config* configuracion){
	tablaPaginasInvertidas = list_create();
	tablaDeArchivos = list_create();
	idPagina = 0;
	idMarco = 0;
	tamanioPagina = config_get_int_value(configuracion, "TAM_PAGINA");
	cantidadMarcosTotales = tamanioMemoria / (tamanioPagina * tamanioLinea);
	cargarTabla();
	inicializarMutex(&mutexArchivos);
	inicializarMutex(&mutexPaginasInvertidas);
}

respuestaDeObtencionDeMemoria* obtenerDatosInvertida(int idDTB, char* nombreArchivo){
	respuestaDeObtencionDeMemoria* respuesta = malloc(sizeof(respuestaDeObtencionDeMemoria));
	t_list* marcosConEseArchivo = filtrarPorDTBYArchivo(idDTB, nombreArchivo);
	int cantidadLineas = cantidadDeLineasArchivo(idDTB, nombreArchivo);
	if(cantidadLineas != -1){
		char* archivo = string_new();
		for(int i=0; i < marcosConEseArchivo->elements_count; i++){
			ElementoTablaInvertida* pagina = list_get(marcosConEseArchivo, i);

			int desplazamiento = storage + pagina->marco * (tamanioPagina * tamanioLinea); //porque el tamanio de pag esta en lineas
			int hastaLinea;
			if(marcosConEseArchivo->elements_count - 1 == i) //Es la ultima pagina
				hastaLinea = cantidadLineas % tamanioPagina;
			else
				hastaLinea = tamanioPagina;

			for (int j = 0;  j < hastaLinea; j++) {
				char* lineaConBasura = malloc(tamanioLinea);
				memcpy(lineaConBasura, desplazamiento + j * tamanioLinea, tamanioLinea);
				if(lineaConBasura[0] != '\n'){
					char** lineaSinBasura = string_split(lineaConBasura, "\n");
					string_append(&archivo, lineaSinBasura[0]);
					freeLineasBasura(lineaSinBasura, lineaConBasura);
				}else{
					free(lineaConBasura);
				}
				string_append(&archivo, "\n");
			}
		}
		respuesta->datos = malloc(strlen(archivo) + 1);
		memcpy(respuesta->datos, archivo, strlen(archivo) + 1);
		return respuesta;
	}else{
		log_error(logger, "Error buscando archivo %s", nombreArchivo);
		return FALLO_DE_SEGMENTO_MEMORIA; //ERROR
	}

}

int asignarDatosInvertida(int idDTB, char* nombreArchivo, int numeroLinea, char* datos){
	numeroLinea--;
	waitMutex(&mutexArchivos);
	int cantidadLineas = cantidadDeLineasArchivo(idDTB, nombreArchivo);
	signalMutex(&mutexArchivos);
	t_list* marcosParaEseArchivo = filtrarPorDTBYArchivo(idDTB, nombreArchivo);

	if(numeroLinea < (cantidadLineas - 1)){
		int paginaDondeSeEncuentraLaLinea = numeroLinea / tamanioPagina;
		int lineaDentroDeLaPagina = numeroLinea % tamanioPagina;
		ElementoTablaInvertida* elementoTabla = list_get(marcosParaEseArchivo, paginaDondeSeEncuentraLaLinea);
		int desplazamientoMarco = elementoTabla->marco * tamanioPagina * tamanioLinea; //Porque el tamanioPagina esta en lineas
		int desplazamientoLinea = lineaDentroDeLaPagina * tamanioLinea;

		if((strlen(datos) + 1) < tamanioLinea){ //Lo que ya estaba, los datos nuevos, el /n y el espacio en el medio
			//Se puede escribir
			log_debug("Linea que va a asignarse: %s", datos);
			char* lineaAGuardar = malloc(tamanioLinea);
			memcpy(lineaAGuardar, datos, strlen(datos) + 1);
			string_trim(&lineaAGuardar);
			memcpy(storage + desplazamientoMarco + desplazamientoLinea, lineaAGuardar, tamanioLinea);
			free(lineaAGuardar);
			log_debug(logger, "Asignados datos con exito");
			return 0;

		}else{
			log_error(logger, "No hay suficiente espacio en la linea %d del archivo %s", (numeroLinea+1), nombreArchivo);
			return ESPACIO_INSUFICIENTE_EN_FM9; //ERROR
		}
	}else {
		if(cantidadLineas - 1 == numeroLinea){
			log_error(logger, "No se puede escribir en la ultima linea del archivo");
		}else{
			log_error(logger, "El archivo no posee la linea %d", numeroLinea + 1);
		}

		return FALLO_DE_SEGMENTO_MEMORIA; //ERROR
	}
}

static int cantidadDeLineasArchivo(int idDTB, char* nombreArchivo){
	bool coincidenIdyArchivo(ElementoArchivos* elemento){
		return elemento->idDTB == idDTB && (strcmp(elemento->nombreArchivo, nombreArchivo) == 0);
	}

	ElementoArchivos* elemento = list_find(tablaDeArchivos, coincidenIdyArchivo);

	if(elemento != NULL)
		return elemento->cantidadLineas;
	else
		return -1;
}

static t_list* filtrarPorDTBYArchivo(int idDTB, char* nombreArchivo){
	bool coincidenIdyArchivo(ElementoTablaInvertida* elemento){
		return elemento->idDTB == idDTB && (strcmp(elemento->nombreArchivo, nombreArchivo) == 0);
	}

	bool compararPorPagina(ElementoTablaInvertida* elem1, ElementoTablaInvertida* elem2){
		return elem1->pagina < elem2->pagina;
	}

	waitMutex(&mutexPaginasInvertidas);
	t_list* lista = list_filter(tablaPaginasInvertidas, coincidenIdyArchivo);
	list_sort(lista, compararPorPagina);
	signalMutex(&mutexPaginasInvertidas);

	return lista;
}

void liberarMemoriaInvertida(int idDTB, char* nombreArchivo){
	void liberarMarcos(ElementoTablaInvertida* elemento){
		if(elemento->idDTB == idDTB && (strcmp(elemento->nombreArchivo, nombreArchivo) == 0)){
			elemento->idDTB = NULL;
			elemento->pagina = -1;
			free(elemento->nombreArchivo);
		}
	}

	bool coincideIdYNombre(ElementoArchivos* elemento){
		return elemento->idDTB == idDTB && (strcmp(elemento->nombreArchivo, nombreArchivo) == 0);
	}

	void destruir(ElementoArchivos* elemento){
		free(elemento->nombreArchivo);
		free(elemento);
	}

	waitMutex(&mutexArchivos);
	list_remove_and_destroy_by_condition(tablaDeArchivos, coincideIdYNombre, destruir);
	signalMutex(&mutexArchivos);

	waitMutex(&mutexPaginasInvertidas);
	list_iterate(tablaPaginasInvertidas, liberarMarcos);
	signalMutex(&mutexPaginasInvertidas);

	log_info(logger, "Borrado archivo %s de memoria", nombreArchivo);
}

void liberarDTBDeMemoriaInvertida(int idDTB){
	void liberar(ElementoTablaInvertida* elemento){
		if(elemento->idDTB == idDTB){
			elemento->idDTB = NULL;
			elemento->pagina = -1;
			free(elemento->nombreArchivo);
		}
	}

	bool coincideId(ElementoArchivos* elemento){
			return elemento->idDTB == idDTB;
		}

		void destruir(ElementoArchivos* elemento){
			free(elemento->nombreArchivo);
			free(elemento);
		}

		int cant = 0;
		void sumar(ElementoArchivos* elemento){
			if(elemento->idDTB == idDTB)
				cant++;
		}
		waitMutex(&mutexArchivos);
		list_iterate(tablaDeArchivos, sumar);
		signalMutex(&mutexArchivos);

		for(int i=0; i<cant; i++){
			waitMutex(&mutexArchivos);
			list_remove_and_destroy_by_condition(tablaDeArchivos, coincideId, destruir);
			signalMutex(&mutexArchivos);
		}

	waitMutex(&mutexPaginasInvertidas);
	list_iterate(tablaPaginasInvertidas, liberar);
	signalMutex(&mutexPaginasInvertidas);

	log_info(logger, "Borrado DTB %d de memoria por pasar a EXIT", idDTB);
}

respuestaDeObtencionDeMemoria* obtenerLineaInvertida(int idDTB, char* nombreArchivo, int numeroLinea){
	respuestaDeObtencionDeMemoria* respuesta = malloc(sizeof(respuestaDeObtencionDeMemoria));
	int cantLineas = cantidadDeLineasArchivo(idDTB, nombreArchivo);
	if(numeroLinea == (cantLineas - 1))
		respuesta->pudoObtener = 3;

	if(numeroLinea < (cantLineas - 1)){
		t_list* marcosConEseArchivo = filtrarPorDTBYArchivo(idDTB, nombreArchivo);
		int paginaDondeSeEncuentraLaLinea = numeroLinea / tamanioPagina;
		int lineaDentroDeLaPagina = numeroLinea % tamanioPagina;
		ElementoTablaInvertida* pagina = list_get(marcosConEseArchivo, paginaDondeSeEncuentraLaLinea);
		int desplazamientoPagina = pagina->marco * tamanioPagina * tamanioLinea; //Porque el tamanioPagina esta en lineas
		int desplazamientoLinea = lineaDentroDeLaPagina * tamanioLinea;
		char* lineaConBasura = asignarMemoria(tamanioLinea);
		memcpy(lineaConBasura, storage + desplazamientoPagina + desplazamientoLinea, tamanioLinea);
		if(lineaConBasura[0]=='\n' || lineaConBasura[0] == NULL){ //FIN DE ARCHIVO
			respuesta->pudoObtener = 3;
			free(lineaConBasura);
		}else{
			char** lineaSinBasura = string_split(lineaConBasura, "\n");
			log_debug(logger, "Linea: %s", lineaSinBasura[0]);
			respuesta->datos = string_new();
			respuesta->pudoObtener = 0;
			string_append(&respuesta->datos, lineaSinBasura[0]);
			freeLineasBasura(lineaSinBasura, lineaConBasura);
		}

		list_destroy(marcosConEseArchivo);
	}else if(numeroLinea != (cantLineas -1)){
		log_error(logger, "Error: El archivo no posee la linea %d", numeroLinea);
		respuesta->pudoObtener = FALLO_DE_SEGMENTO_MEMORIA;
	}
	return respuesta;
}

static void cargarElemento(ElementoTablaInvertida* elemento, int pagina, char* nombreArchivo, int idDTB){
	elemento->pagina = pagina;
	elemento->idDTB = idDTB;
	elemento->nombreArchivo = malloc(strlen(nombreArchivo) + 1);
	memcpy(elemento->nombreArchivo, nombreArchivo, strlen(nombreArchivo) + 1);
}

static ElementoArchivos* crearElementoArchivos(int idDTB, char* nombreArchivo, int lineas){
	ElementoArchivos* elemento = malloc(sizeof(ElementoArchivos));
	elemento->idDTB = idDTB;
	elemento->nombreArchivo = string_new();
	string_append(&elemento->nombreArchivo, nombreArchivo);
	elemento->cantidadLineas = lineas;
}

RespuestaGuardado* guardarDatosInvertida(int idDTB, char* datos, char* nombreArchivo){
	log_debug(logger, "Guardando en paginas invertidas");

	RespuestaGuardado* respuesta = malloc(sizeof(RespuestaCargaInvertida));
	int totalLineas = cantidadDeLineas(datos);
	char** lineas = dividirPorLineas(datos);
	int cantidadPaginas = 1;
	int lineasEnLaUltimaPagina = totalLineas % tamanioPagina;
	if (totalLineas == tamanioPagina) lineasEnLaUltimaPagina = tamanioPagina;
	if(totalLineas > tamanioPagina){
		cantidadPaginas = totalLineas / tamanioPagina;
		if(lineasEnLaUltimaPagina != 0)
				cantidadPaginas++;
	}

	if(lineasEnLaUltimaPagina == 0)
			lineasEnLaUltimaPagina = tamanioPagina - 1;

	ElementoArchivos* elementoArchivo = crearElementoArchivos(idDTB, nombreArchivo, totalLineas);
	list_add(tablaDeArchivos, elementoArchivo);

	int lineaACargar = 0;
	for(int i = 0; i < cantidadPaginas; i++){
		char* textoAGuardar;
		int marcoAUtilizar = obtenerMarcoLibreInvertida();
		log_error(logger, "Se guarda la pagina %d de los datos del DTB %d en el marco %d", i, idDTB, marcoAUtilizar);
		if(marcoAUtilizar == -1){
			freeLineas(lineas);
			respuesta->pudoGuardar = ESPACIO_INSUFICIENTE_EN_FM9; //ERROR NO HAY MARCOS LIBRES
			return respuesta;
		}else{
			respuesta->pudoGuardar = 0; //No hay error

			ElementoTablaInvertida* elementoInvertida = obtenerElementoPorMarco(marcoAUtilizar);
			cargarElemento(elementoInvertida, idPagina, nombreArchivo, idDTB);

			idPagina++;
			int base = storage + marcoAUtilizar * (tamanioPagina * tamanioLinea); //Porque el tamanioPagina esta en lineas
			if(cantidadPaginas - 1 == i){ //Es la ultima pagina
				for (int j = 0;  j < lineasEnLaUltimaPagina; j++) {
					if(lineas[lineaACargar]==NULL)
						lineas[lineaACargar] = string_new();

					string_append(&lineas[lineaACargar], "\n");
					char* textoAEscribir = malloc(tamanioLinea);
					memcpy(textoAEscribir, lineas[lineaACargar], strlen(lineas[lineaACargar]) + 1);
					memcpy(base + tamanioLinea * j, textoAEscribir, tamanioLinea); //Guardando de a una linea
					free(textoAEscribir);
					lineaACargar++;
				}
			}else{
				for(int j = 0; j < (tamanioPagina); j++){
					if(lineas[lineaACargar]==NULL)
						lineas[lineaACargar] = string_new();

					string_append(&lineas[lineaACargar], "\n");
					char* textoAEscribir = malloc(tamanioLinea);
					memcpy(textoAEscribir, lineas[lineaACargar], strlen(lineas[lineaACargar]) + 1);
					memcpy(base + tamanioLinea * j, textoAEscribir, tamanioLinea); //Guardando de a una linea
					free(textoAEscribir);
					lineaACargar++;
				}
			}

		  }
		}
			log_debug(logger, "Datos guardados");
			respuesta->pesoArchivo = cantidadPaginas * tamanioPagina;


		freeLineas(lineas);

	return respuesta;
}

void dumpInvertida(int idDTB){
	log_info(logger, "Dump de DTB: %d", idDTB);
	char* dump = string_new();

	for(int i=0; i < (tablaDeArchivos->elements_count); i++){
		waitMutex(&mutexArchivos);
		ElementoArchivos* elemento = list_get(tablaDeArchivos, i);
		signalMutex(&mutexArchivos);

		if(elemento->idDTB == idDTB){
			int cantidadPaginas = elemento->cantidadLineas / tamanioPagina + 1;
			respuestaDeObtencionDeMemoria* datos = obtenerDatosInvertida(idDTB, elemento->nombreArchivo);

			string_append_with_format(&dump, "Abierto archivo: %s \n"
							">>> Ocupa %d paginas \n"
							">>> Su contenido es: %s", elemento->nombreArchivo, cantidadPaginas, datos->datos);
		}
	}

	if(strcmp(dump, "") != 0){
		log_info(logger, "- %s", dump);
	}else{
		log_info(logger, "DUMP ----- No hay archivos abiertos para ese DTB.");
	}

	free(dump);
}

static ElementoTablaInvertida* obtenerElementoPorMarco(int marco){
	bool coincideMarco(ElementoTablaInvertida* elemento){
		return elemento->marco == marco;
	}

	waitMutex(&mutexPaginasInvertidas);
	ElementoTablaInvertida* elemento = list_find(tablaPaginasInvertidas, coincideMarco);
	signalMutex(&mutexPaginasInvertidas);

	return elemento;
}

static ElementoTablaInvertida* obtenerElementoPorIdDTB(int idDTB){
	bool coincideId(ElementoTablaInvertida* elemento){
		return elemento->idDTB == idDTB;
	}

	waitMutex(&mutexPaginasInvertidas);
	ElementoTablaInvertida* elemento = list_find(tablaPaginasInvertidas, coincideId);
	signalMutex(&mutexPaginasInvertidas);

	return elemento;
}

static int obtenerMarcoLibreInvertida(){
	bool marcoEstaLibre(ElementoTablaInvertida* elemento){
		return elemento->pagina == -1;
	}

	waitMutex(&mutexPaginasInvertidas);
	ElementoTablaInvertida* elemento = list_find(tablaPaginasInvertidas, marcoEstaLibre);
	signalMutex(&mutexPaginasInvertidas);

	if(elemento != NULL){
		return elemento->marco;
	}else{
		log_error(logger, "No quedan marcos disponibles");
		return -1;
	}
}

static ElementoTablaInvertida* crearElementoTablaInvertida(){
	ElementoTablaInvertida* elemento = malloc(sizeof(ElementoTablaInvertida));
	elemento->marco = idMarco;
	idMarco++;
	elemento->idDTB = NULL;
	elemento->nombreArchivo = NULL;
	elemento->pagina = -1;
	return elemento;
}

static void cargarTabla(){
	waitMutex(&mutexPaginasInvertidas);
	for(int i=0; i < cantidadMarcosTotales; i++){
		ElementoTablaInvertida* elemento = crearElementoTablaInvertida();
		list_add(tablaPaginasInvertidas, elemento);
	}
	signalMutex(&mutexPaginasInvertidas);
}
