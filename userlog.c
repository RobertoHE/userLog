/********************************************************************************/
/*							FICHEROS .H INCLUIDOS								*/
/********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>

#include <syslog.h>

#include "userlog.h"


/******************************************************************************/
/*								CONSTANTES									  */
/******************************************************************************/


/******************************************************************************/
/*							   TIPOS LOCALES							  	  */
/******************************************************************************/

/******************************************************************************/
/*							 VARIABLES EXTERNAS							  	  */
/******************************************************************************/

/********************************************************************************/
/*						VARIABLES GLOBALES										*/
/********************************************************************************/
const char logfile[] = USERLOG_PATH;
const char oldfile[] = USERLOG_PATH_OLD;

const char cmdfile[] = CMDLOG_PATH;

unsigned int log_msk_console = LOG_UPTO(USERLOG_MSK_CONSOLE); // Todas
unsigned int log_msk_file = LOG_UPTO(USERLOG_MSK_FILE);		  // Solo error y notificaciones

// Para encadenar y datar
char msglog[1024];
char cmdlog[512];
char cmdsys[128];

// Para datar
long seg_log;
struct tm dt_log;
struct timespec stime_log;

unsigned char fl_debug = normalM;

// Senal de actualizacion
struct sigaction accion_log;

/******************************************************************************/
/*							FUNCIONES LOCALES								  */
/******************************************************************************/

/******************************************************************************/
void userlog_updater()
{
	switch (fl_debug) // update and evaluate
	{
	default:
		fl_debug = silentM;
	case silentM:
		userlog(LOG_NOTICE, "SIGHUP -> Userlog Silent Mode\n");
		log_msk_console = 0; // Nada
		log_msk_file = 0;	 // Nada
		break;
	case normalM:
		log_msk_console = LOG_UP(USERLOG_MSK_CONSOLE);
		log_msk_file = LOG_UPTO(USERLOG_MSK_FILE);
		userlog(LOG_NOTICE, "SIGHUP -> Userlog Release Mode\n");
		break;
	case terminalM:
		log_msk_console = LOG_UP(USERLOG_MSK_CONSOLE);
		log_msk_file = 0;
		userlog(LOG_NOTICE, "SIGHUP -> Userlog Only Console\n");
		break;
	case fileM:
		log_msk_console = 0;
		log_msk_file = LOG_UPTO(USERLOG_MSK_FILE);
		userlog(LOG_NOTICE, "SIGHUP -> Userlog Only file\n");
		break;
	case ultraverboseM:
		log_msk_console = LOG_UP(LOG_DEBUG);
		log_msk_file = LOG_UPTO(LOG_DEBUG);
		userlog(LOG_NOTICE, "SIGHUP -> Userlog Ultraverbose Mode\n");
		break;
	}
}


/******************************************************************************/
void manejador_sighup(int signal)
/*Si el programa recibe una senal sighup externa del sistema operativo, modifica la salida por fichero o terminal*/
{
	printf("### UserLog Config ###\n");
	++fl_debug;
	userlog_updater();
}

/******************************************************************************/
void initlog(debugModes_t mode)
/******************************************************************************/
{
	FILE *fMarca;
	char data;

	accion_log.sa_flags = 0;
	accion_log.sa_flags = SA_RESTART;
	accion_log.sa_handler = manejador_sighup;
	sigemptyset(&accion_log.sa_mask);
	sigaction(SIGHUP, &accion_log, NULL);
	//
	fl_debug = mode;
	// Comprueba si activas por fichero
	if ((fMarca = fopen(USERLOG_ENABLE_FILE, "rb")) != NULL)
	{
		// Lee 1 byte
		if (fread(&data, 1, 1, fMarca) == 1)
		{
			data = data - '0';
			if (data > ultraverboseM || data < 0)
			{
				data = normalM;
			}
			fl_debug = data:
		}
		else
			fl_debug = normalM;

		// Lo cierra
		fclose(fMarca);
	}

	// update manual
	userlog_updater();

	return;
}

/********************************************************************************/
/*							FUNCIONES DEL MODULO								*/
/********************************************************************************/

/******************************************************************************/
int userlog(unsigned int priority, const char *format, ...)
/******************************************************************************/
{
	va_list ap;
	int res = 0;
	FILE *fd;
	int fl_limite = 0;

	// Si no va a trazar nada...
	if (!((LOG_MASK(priority) & log_msk_console) || (LOG_MASK(priority) & log_msk_file)))
	{
		return res;
	}

	va_start(ap, format); // Captura de argumentos multiples

	// Traza a consola, va sin datar tal cual
	if (LOG_MASK(priority) & log_msk_console)
	{
		res = vprintf(format, ap);
		fflush(stdout);
	}

	// Traza a fichero, va datada
	if (LOG_MASK(priority) & log_msk_file)
	{
		// Leemos reloj en UTC
		time(&seg_log);

		// Lo pasamos a estructura
		localtime_r(&seg_log, &dt_log);

		// Genera la cadena fecha-hora
		sprintf(msglog, "%02d/%02d/%02d %02d:%02d:%02d - ",
				dt_log.tm_mday,
				dt_log.tm_mon + 1,
				dt_log.tm_year - 100, // Viene referido a 1900
				dt_log.tm_hour,
				dt_log.tm_min,
				dt_log.tm_sec);

		// Antepone la fecha
		strncat(msglog, format, 1023 - strlen(msglog));

		if ((fd = fopen(logfile, "a")) == NULL)
		{
			printf("FAIL\n");
		}
		else
		{
			vfprintf(fd, msglog, ap);

			// Control de tamano, en bytes
			if (ftell(fd) >= MAX_SIZE_LOG_FILE)
			{

				fl_limite = 1;
			}
			fflush(fd);
			fsync(fileno(fd));
			fclose(fd);
		}
		// Guarda como .old el actual y crea uno nuevo vacio
		if (fl_limite)
		{
			// Renombra el actual a .old
			sprintf(cmdsys, "mv -f %s %s", logfile, oldfile);
			// printf("renombra: %s\n",cmdsys);
			system(cmdsys);

			// crea vacio
			sprintf(cmdsys, "touch  %s", logfile);
			system(cmdsys);
		}
	}

	va_end(ap);

	return res;
}

/******************************************************************************/
int logcmd(unsigned char datar, unsigned char lf, const char *format, ...)
/******************************************************************************/
{
	va_list ap;
	int res = 0;
	FILE *fd;
	int fl_limite = 0;
	char *pout;
	unsigned int i;

	va_start(ap, format); // Captura de argumentos multiples

	if (datar)
	{
		// Leemos reloj
		clock_gettime(CLOCK_REALTIME, &stime_log);

		// Leemos reloj en UTC
		time(&seg_log);

		// Lo pasamos a estructura
		localtime_r(&seg_log, &dt_log);

		// Genera la cadena fecha-hora
		sprintf(msglog, "%02d/%02d %02d:%02d:%02d.%03ld - ",
				dt_log.tm_mday,
				dt_log.tm_mon + 1,
				/*dt_log.tm_year - 100, // Viene referido a 1900*/
				dt_log.tm_hour,
				dt_log.tm_min,
				dt_log.tm_sec,
				stime_log.tv_nsec / 1000000);

		// Antepone la fecha
		strncat(msglog, format, 1023 - strlen(msglog));

		vsprintf(cmdlog, msglog, ap);
	}
	else
		vsprintf(cmdlog, format, ap);

	//
	// Formato caracteres no printables
	pout = msglog;

	for (i = 0; i < strlen(cmdlog); i++)
	{
		if (cmdlog[i] >= ' ')
		{
			pout[0] = cmdlog[i];
			++pout;
		}
		else
			pout += sprintf(&pout[0], "<0x%02X>", cmdlog[i]);
	}

	// Cierra cadena
	if (lf)
	{
		pout[0] = '\n';
		pout[1] = 0;
	}
	else
		pout[0] = 0;

	if ((fd = fopen(cmdfile, "a")) != NULL)
	{
		fprintf(fd, msglog);

		// Control de tamano, en bytes
		if (ftell(fd) >= MAX_SIZE_CMD_FILE)
			fl_limite = 1;

		fflush(fd);
		fsync(fileno(fd));
		fclose(fd);

		// Guarda como .old el actual y crea uno nuevo vacio
		if (fl_limite)
		{
			for (i = NUM_CMD_FILE; i > 0; i--)
			{
				sprintf(cmdsys, "mv %s.old%d %s.old%d", cmdfile, i - 1, cmdfile, i);
				system(cmdsys);
			}
			if (NUM_CMD_FILE)
			{
				// Renombra el actual a .old0
				sprintf(cmdsys, "mv %s %s.old0", cmdfile, cmdfile);
				system(cmdsys);
			}

			// crea vacio
			sprintf(cmdsys, "touch  %s", cmdfile);
			system(cmdsys);
		}
	}

	va_end(ap);

	return res;
} /* logcmd */
