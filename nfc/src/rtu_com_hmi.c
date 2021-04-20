#include "rtu_com_hmi.h"

#include <stdlib.h>
#include <stdbool.h>

#include "lwip/opt.h"
#include "debug.h"
#include "lift.h"
#include "mot_pap.h"
#include "relay.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"
#include <string.h>
#include "json_wp.h"
#include "net_commands.h"

static void prvEmergencyStop(void);
static void prvNetconnError(err_t err);

int retransm(struct netconn *newconn, struct netbuf *buf)
{
	u16_t len;
	err_t err;
	char *rx_buffer, *tx_buffer;
	do {
		if ((err = netbuf_data(buf, (void**) &rx_buffer, &len)) != ERR_OK) {
			goto error_free_tx_buffer;
		}

		rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string

		int ack_len = json_wp(rx_buffer, &tx_buffer);

		char ack_buff[5];
		sprintf(ack_buff, "%04x", ack_len);

		if ((err = netconn_write(newconn, &ack_buff, 4, NETCONN_COPY))
				!= ERR_OK) {
			goto error_free_tx_buffer;
		}

		if ((err = netconn_write(newconn, tx_buffer, ack_len, NETCONN_COPY))
				!= ERR_OK) {
			goto error_free_tx_buffer;
		}

		vPortFree(tx_buffer);
		tx_buffer = NULL;

	} while (netbuf_next(buf) >= 0);
	return 0;

error_free_tx_buffer:
	printf("netconn: error \"%s\"\n", lwip_strerr(err));
	vPortFree(tx_buffer);
	tx_buffer = NULL;
	return -1;
}

/*-----------------------------------------------------------------------------------*/
static void tcp_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);
	struct netconn *conn, *newconn;
	struct netbuf *buf;
	err_t err;
	//void *unused;

	/* Nuevo identificador de conexion -conn- */
	conn = netconn_new(NETCONN_TCP);

	/* Enlace de la conexión en puerto 5020 */
	netconn_bind(conn, NULL, 5020);

	/* Socket generado en modo escucha */
	netconn_listen(conn);
	lDebug(Info, "Listening on port: 5020 \n\r");

	while (1) {
		/* Aceptar nueva conexion */
		if ((err = netconn_accept(conn, &newconn)) == ERR_OK) {
			newconn->recv_timeout = RCV_TIMEO;

			while ((err = netconn_recv(newconn, &buf)) == ERR_OK) {
				retransm(newconn, buf);
				netbuf_delete(buf);
			}

			/* Close connection and discard connection identifier. */
			netconn_close(newconn);
			netconn_delete(newconn);
		}

		lDebug(Error, "Error en funcion NETCONN -netbuf_data-");
		//prvNetconnError(err);
		//prvEmergencyStop();
		lDebug(Info, "	- ** 	Desconexion RTU 	** - ");
	}
}
/*-----------------------------------------------------------------------------------*/
void stackIp_ThreadInit(void)
{
	sys_thread_new("tcp_thread", tcp_thread, NULL, DEFAULT_THREAD_STACKSIZE,
	configMAX_PRIORITIES - 2);
}
/*-----------------------------------------------------------------------------------*/
/**
 * @brief 	Only prints out errors related with NETCONN LWIP module.
 * @param 	LWIP error code
 * @return	never
 * @note	Only prints. Variable iServerStatus for hanlde error events, #define ERROR_NETCONN 0x84.
 */
static void prvNetconnError(err_t err)
{
	switch (err) {
	case (ERR_TIMEOUT):
		lDebug(Error, "\n ENET - RecvTimeOut - Se detienen procesos!! \n");
		break;
	case (ERR_ARG):
		lDebug(Error,
				"\n ENET - Argumento de funcion -netconn_recv- ilegal - Se detienen procesos!! \n");
		break;
	case (ERR_CONN):
		lDebug(Error, "\n Problemas de conexion - Se detienen procesos!! \n");
		break;
	case (ERR_CLSD):
		lDebug(Error, "\n Closed Connection - Se detienen procesos!! \n");
		break;
	case (ERR_BUF):
		lDebug(Error, "\n Error al generar Buffer - Se detienen procesos!! \n");
		break;
	}
	return;
}

#endif /* LWIP_NETCONN */

///*-----------------------------------------------------------*/
/**
 * @brief 	Stops movements for all three axis on exit ethernet loop with HMI.
 * @param 	none.
 * @return	never.
 * @note	Stops movements on HMI communication lost.
 */
static void prvEmergencyStop(void)
{
	struct mot_pap_msg *pArmMsg;
	struct mot_pap_msg *pPoleMsg;
	struct lift_msg *pLiftMsg;

	pArmMsg = (struct mot_pap_msg*) pvPortMalloc(sizeof(struct mot_pap_msg));
	pArmMsg->type = MOT_PAP_TYPE_STOP;
	if (xQueueSend(arm_queue, &pArmMsg, portMAX_DELAY) == pdPASS) {
		lDebug(Debug, " Comando enviado a arm.c exitoso!");
	} else {
		lDebug(Error, "Comando NO PUDO ser enviado a arm.c");
	}

	pPoleMsg = (struct mot_pap_msg*) pvPortMalloc(sizeof(struct mot_pap_msg));
	pPoleMsg->type = MOT_PAP_TYPE_STOP;
	if (xQueueSend(pole_queue, &pPoleMsg, portMAX_DELAY) == pdPASS) {
		lDebug(Debug, "Comando enviado a pole.c exitoso!");
	} else {
		lDebug(Error, "Comando NO PUDO ser enviado a pole.c");
	}

	pLiftMsg = (struct lift_msg*) pvPortMalloc(sizeof(struct lift_msg));
	pLiftMsg->type = LIFT_TYPE_STOP;
	if (xQueueSend(lift_queue, &pLiftMsg, portMAX_DELAY) == pdPASS) {
		lDebug(Debug, "Comando enviado a lift.c exitoso!");
	} else {
		lDebug(Error, "Comando NO PUDO ser enviado a lift.c");
	}

	relay_main_pwr(false);

	return;

}
