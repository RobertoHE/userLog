#ifndef _LOG_H_
#define _LOG_H_

#include <syslog.h>

/******************************************************************************/
/*								CONSTANTES									  */
/******************************************************************************/

// #define _TRAZA_

#ifdef _TRAZA_
#define USERLOG(prio, fmt, args...) userlog(prio, fmt, ##args)
#else
#define USERLOG(prio, fmt, args...)
#endif

/* Definidos en syslog.h -> */
/* LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG */
/* Macros: LOG_MASK(...), LOG_UPTO(...) */
#ifndef USERLOG_MSK_CONSOLE
#warning USERLOG_MSK_CONSOLE Default value LOG_DEBUG
#define USERLOG_MSK_CONSOLE LOG_DEBUG
#endif

#ifndef USERLOG_MSK_FILE
#warning USERLOG_MSK_FILE Default value LOG_NOTICE
#define USERLOG_MSK_FILE LOG_NOTICE
#endif

// File paths
#ifndef USERLOG_ENABLE_FILE
#define USERLOG_ENABLE_FILE "/log/trazas"
#endif

#ifndef USERLOG_PATH
#define USERLOG_PATH "/log/userlogs.log"
#endif

#ifndef USERLOG_PATH_OLD
#define USERLOG_PATH_OLD "/log/old/userlogs.log"
#endif

#ifndef CMDLOG_PATH
#define CMDLOG_PATH "/log/CMDlogs.log"
#endif

#ifndef NUM_CMD_FILE
#define NUM_CMD_FILE 1
#endif

// Tamano maximo del fichero de trazas (los ficheros pueden ser algo mayores para no truncar los mensajes)
#ifndef MAX_SIZE_LOG_FILE
#define MAX_SIZE_LOG_FILE 0x40000 // 256 kbytes
#endif

#ifndef MAX_SIZE_CMD_FILE
#define MAX_SIZE_CMD_FILE 0x80000 // 512 kbytes
#endif

typedef enum DEBUGMODES
{
	silentM = 0,
	normalM,
	terminalM,
	fileM,
	ultraverboseM
} debugModes_t; // Para conmutar entre modos: debug pantalla, debug fichero o debug por ambos

/********************************************************************************/
/*						VARIABLES GLOBALES										*/
/********************************************************************************/

/********************************************************************************/
/*							FUNCIONES DEL MODULO								*/
/********************************************************************************/
void initlog(debugModes_t mode);
int userlog(unsigned int priority, const char *format, ...);
int logcmd(unsigned char datar, unsigned char lf, const char *format, ...);

#endif // _LOG_H_
