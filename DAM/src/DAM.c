#include "DAM.h"

void inicializarDAM(){
	inicializarMutex(&mutexColaOperaciones);
	inicializarSem(&semHayEnColaOperaciones, 0);
	inicializarSem(&semProductores, 0);
	colaOperaciones = queue_create();
	configuraciones = config_create(ARCHIVO_CONFIGURACION);
	transferSize = config_get_int_value(configuraciones, "TRANSFER_SIZE");
	logger = crearLogger(ARCHIVO_LOG, "DAM");
}

void entenderMensaje(int emisor, char header){
	int idDTB = deserializarInt(emisor);
	char* path = deserializarString(emisor);
	int cantidadDeLineas;
	switch(header){
		case CARGAR_ESCRIPTORIO_EN_MEMORIA:
			log_info(logger, "Ehhh, voy a buscar %s para %d", path, idDTB);
			int validarArchivo = validarArchivoMDJ(path);
			if(validarArchivo != 0){
				log_info(logger, "Enviando error a SAFA para id del DTB %d con el path %s y el error %d", idDTB, path, validarArchivo);
				enviarError(idDTB, path, validarArchivo);
			}else {
				char* datos = obtenerDatosDeMDJ(path);
				int estadoDeCarga = enviarDatosAFM9(path, datos);
				if(estadoDeCarga != 0){
					log_info(logger, "Enviando error a SAFA para id del DTB %d con el path %s y el error %d", idDTB, path, estadoDeCarga);
					enviarError(idDTB, path, estadoDeCarga);
				}else{

					log_info(logger, "Enviando exito a SAFA para el DTB %d con el path %s", idDTB, path);
					notificarASafaExito(CARGADO_CON_EXITO_EN_MEMORIA,idDTB, path);
				}
			}
			break;
		case GUARDAR_ESCRIPTORIO:
			log_debug(logger, "Guardando escriptorio");
			pedirDatosAFM9(path);
			cantidadDeLineas = deserializarInt(socketFM9);
			char* datos = recibirFlushFM9(cantidadDeLineas);
			log_info(logger, "Recibi flush de FM9");
			int guardarDatos = guardarDatosEnMDJ(datos, path, cantidadDeLineas);
			if(guardarDatos != 0){
				log_info(logger, "Enviando error a SAFA para id del DTB %d con el path %s y el error %d", idDTB, path, guardarDatos);
				enviarError(idDTB, path, guardarDatos);
			}else{
				log_info(logger, "Enviando exito a SAFA para el DTB %d con el path %s", idDTB, path);
				notificarASafaExito(GUARDADO_CON_EXITO_EN_MDJ,idDTB, path);
			}
			free(datos);
			break;
		case CREAR_ARCHIVO:
			log_debug(logger, "creando archivo");
			cantidadDeLineas = deserializarInt(emisor);
			int crearArchivo = crearArchivoEnMDJ(socketMDJ, path, cantidadDeLineas);
			if(crearArchivo != 0){
				log_info(logger, "Enviando error a SAFA para id del DTB %d con el path %s y el error %d", idDTB, path, crearArchivo);
				enviarError(idDTB, path, crearArchivo);
			}else{
				log_info(logger, "Enviando creado archivo con exito a SAFA para el DTB %d con el path %s", idDTB, path);
				notificarASafaExito(CREADO_CON_EXITO_EN_MDJ,idDTB, path);
			}
			break;
		case BORRAR_ARCHIVO:
			log_debug(logger, "borrar archivo");
			int borrarArchivo = borrarArchivoEnMDJ(path);
			if(borrarArchivo != 0){
				log_info(logger, "Enviando error a SAFA para id del DTB %d con el path %s y el error %d", idDTB, path, borrarArchivo);
				enviarError(idDTB, path, borrarArchivo);
			}else{
				log_info(logger, "Enviando borrado archivo con exito a SAFA para el DTB %d con el path %s", idDTB, path);
				notificarASafaExito(BORRADO_CON_EXITO_EN_MDJ,idDTB, path);
			}
			break;
		default:
			log_error(logger, "Header desconocido");
	}
}

int identificarse(int emisor, char header){
	if(header == IDENTIFICARSE){
		char identificado = deserializarChar(emisor);
		log_debug(logger, "Handshake de: %s", traducirModulo(identificado));
		if(identificado == CPU)
			return 1;
	}
	log_error(logger, "Conexion rechazada");
	return 0;
}

void crearSelect(int servidor){
	Select* select = asignarMemoria(sizeof(Select));
	select->colaOperaciones = colaOperaciones;
	select->funcionEntenderMensaje = &entenderMensaje;
	select->logger = logger;
	select->mutexOperaciones = &mutexColaOperaciones;
	select->semOperaciones = &semHayEnColaOperaciones;
	select->socket = servidor;
	select->identificarse = &identificarse;
	select->semProductores = &semProductores;
	realizarNuestroSelect(select);
}

int main(void) {
	inicializarDAM();
	t_config* configuracion = config_create(ARCHIVO_CONFIGURACION);

	direccionServidor direccionDAM = levantarDeConfiguracion(NULL, "PUERTO", configuracion);
	int servidorDAM = crearServidor(direccionDAM.puerto, INADDR_ANY);

	//crear servidores para ser cliente de ellos
	direccionServidor direccionSAFA = levantarDeConfiguracion("IP_SAFA", "PUERTO_SAFA", configuracion);
	socketSAFA = conectarConServidor(direccionSAFA.puerto, inet_addr(direccionSAFA.ip));

	direccionServidor direccionMDJ = levantarDeConfiguracion("IP_MDJ", "PUERTO_MDJ", configuracion);
	socketMDJ = conectarConServidor(direccionMDJ.puerto, inet_addr(direccionMDJ.ip));

	direccionServidor direccionFM9 = levantarDeConfiguracion("IP_FM9", "PUERTO_FM9", configuracion);
	socketFM9 = conectarConServidor(direccionFM9.puerto, inet_addr(direccionFM9.ip));

	config_destroy(configuracion);

	//mandar handshakes a los servidores
	handshake(socketSAFA, DAM);
	handshake(socketMDJ, DAM);
	handshake(socketFM9, DAM);

	crearSelect(servidorDAM);

	while(1);
	return 0;
}
