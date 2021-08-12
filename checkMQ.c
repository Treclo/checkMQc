#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmqc.h>
#include <cmqcfc.h>
#include <cmqbc.h>
#include <cmqxc.h>
#include <time.h>

/************************************************************************************************/
/* El programa se debe ejecutar usando															*/                                                                               */
/* nombreEjecutable gestor mensajesOK mensajesWARN mensajesNOTOK puerto1 puerto2 ... puerto3	*/
/************************************************************************************************/

int main (int argc, char *argv[]){
	// Variables para la conexion
	MQHCONN		Hcon;								// connection handle
	MQHOBJ		Hobj;								// object handle
	MQLONG		CompCode;							// completion code
	MQLONG		Reason;								// reason code
	MQLONG		CReason;							// reason code for MQCONNX

	// Variables para la comprobacion
	MQHBAG		adminBag = MQHB_UNUSABLE_HBAG;		// admin bag for mqExecute
	MQHBAG		responseBag = MQHB_UNUSABLE_HBAG;	// response bag for mqExecute
	MQHBAG		qAttrsBag;							// bag containing q attributes
	MQHBAG		errorBag;							// bag containing cmd server error
	MQLONG		qNameLength;						// Actual length of q name
	MQLONG		qDepth;								// depth of queue
	MQLONG		i;									// loop counter
	MQLONG		numberOfBags;						// number of bags in response bag
	MQCHAR		qName[MQ_Q_NAME_LENGTH+1];			// name of queue extracted from bag
	MQLONG		chlNameLength;						// Actual length of channel name
	MQLONG		chlType;							// Type of channel
	MQLONG		chlStatus;							// Status of channel
	MQCHAR		chlName[MQ_OBJECT_NAME_LENGTH+1];	// name of channel extracted from bag
	MQHBAG		cAttrsBag;							// bag containing channel attributes

	// Variables propias para varias cosas
	char		*pntr;								// puntero para la conversion
	int			booleano = 1;						// booleano, 1=TRUE y 0=FALSE
	int			controlChannel = 0;					// Controla si hay algun canal mal
	int			controlqueue = 0;					// Controla si hay alguna cola mal
	int			controlListener = 0;				// Controla si hay alguna listener mal
	char		conversion[100];					// Char para conversion de caracteres
	char		comando[100];						// char para la ejecucion de comandos
	time_t		now = time(NULL);					// variable para coger la fecha
	struct		tm *t = localtime(&now);			// structura que usa la variable now
	char		fecha[100];							// guarda la fecha de ejecucion
	int			j = 0;								// variable para el loop for
	int			k = 0;								// otra variable para un loop
	int			mensajesOK = 0						// < de estos da OK
	int			mensajesWARN = 0					// < de estos da WARN y > da NOTOK
	char		gestor[25];							// nombre del gestor a comprobar

	// Ejecucion no valida con argc menor de (gestor mensajesOK mensajesWARN)
	if (argc < 3){
		printf("Argumentos insuficientes\n");
		exit(1);
	}

	strcpy(gestor, argv[1]);
	mensajesOK = (int)strtol(argv[2], &pntr, 10);
	mensajesWARN = (int)strtol(argv[3], &pntr, 10);

	sprintf(fecha, "%02d/%02d %02d:%02d\n", t->tm_mday, t->tm_mon+1, t->tm_hour, t->tm_min);

	MQCONN(gestor, &Hcon, &CompCode, &CReason);
	if (CompCode == MQCC_FAILED) {
		printf("NOTOK\tConexion fallida, gestor %s no responde por: %d\n", gestor, CReason);
		exit(1);
	}
	
	printf("OK\tConexion realizada correctamente, gestor %s respondiendo\n", gestor);
	mqCreateBag(MQCBO_ADMIN_BAG, &adminBag, &CompCode, &Reason);
	mqCreateBag(MQCBO_ADMIN_BAG, &responseBag, &CompCode, &Reason);
	mqAddString(adminBag, MQCA_Q_NAME, MQBL_NULL_TERMINATED, "*", &CompCode, &Reason);
	mqAddInteger(adminBag, MQIA_Q_TYPE, MQQT_LOCAL, &CompCode, &Reason);
	mqAddInquiry(adminBag, MQIA_CURRENT_Q_DEPTH, &CompCode, &Reason);
	mqExecute(Hcon, MQCMD_INQUIRE_Q, MQHB_NONE, adminBag, responseBag, MQHO_NONE, MQHO_NONE, &CompCode, &Reason);
	mqCountItems(responseBag, MQHA_BAG_HANDLE, &numberOfBags, &CompCode, &Reason);
	for ( i=0; i<numberOfBags; i++) {
		mqInquireBag(responseBag, MQHA_BAG_HANDLE, i, &qAttrsBag, &CompCode, &Reason);
		mqInquireString(qAttrsBag, MQCA_Q_NAME, 0, MQ_Q_NAME_LENGTH, qName, &qNameLength, NULL, &CompCode, &Reason);
		if( strstr(qName, "MQAI") != NULL ) {
			continue;
		}

		mqInquireInteger(qAttrsBag, MQIA_CURRENT_Q_DEPTH, MQIND_NONE, &qDepth, &CompCode, &Reason);
		mqTrim(MQ_Q_NAME_LENGTH, qName, qName, &CompCode, &Reason);
		if( strstr(qName, "DEADLQ") != NULL && qDepth > 0 ){
			printf("NOTOK\tHay mensajes en la cola de la muerte %s\n", qName);
		} else if {
			strstr(qName, "SYSTEM") != NULL
			printf("OK\t%s tiene %d mensajes\n", qName, qDepth);
		}

		if( qDepth < mensajesOK ){
			continue;
		} else if (qDepth >= mensajesOK && qDepth < mensajesWARN) {
			printf("WARN\t%s tiene %d mensajes\n", qName, qDepth);
			controlqueue++;
		} else {
			printf("NOTOK\t%s tiene %d mensajes\n", qName, qDepth);
			controlqueue++;
		}

		if ( controlqueue == 0 ){
			printf("OK\tTodas las colas con menos de 5 mensajes\n");
		}
	}

	mqCreateBag(MQCBO_ADMIN_BAG, &adminBag, &CompCode, &Reason);
	mqCreateBag(MQCBO_ADMIN_BAG, &responseBag, &CompCode, &Reason);
	mqAddString(adminBag, MQCACH_CHANNEL_NAME, MQBL_NULL_TERMINATED, "*", &CompCode, &Reason);
	mqAddInteger(adminBag, MQIACH_CHANNEL_INSTANCE_TYPE, MQOT_CURRENT_CHANNEL, &CompCode, &Reason);
	mqAddInteger(adminBag, MQIACH_CHANNEL_INSTANCE_ATTRS, MQIACF_ALL, &CompCode, &Reason);
	mqExecute(Hcon, MQCMD_INQUIRE_CHANNEL_STATUS, MQHB_NONE, adminBag, responseBag, MQHO_NONE, MQHO_NONE, &CompCode, &Reason);
	mqCountItems(responseBag, MQHA_BAG_HANDLE, &numberOfBags, &CompCode, &Reason);

	for ( i=0; i<numberOfBags; i++) {
		mqInquireBag(responseBag, MQHA_BAG_HANDLE, i, &cAttrsBag, &CompCode, &Reason);
		mqInquireString(cAttrsBag, MQCACH_CHANNEL_NAME, 0, MQ_OBJECT_NAME_LENGTH, chlName, &chlNameLength, NULL, &CompCode, &Reason);
		mqTrim(MQ_CHANNEL_NAME_LENGTH, chlName, chlName, &CompCode, &Reason);
		mqInquireInteger(cAttrsBag, MQIACH_CHANNEL_STATUS, MQIND_NONE, &chlStatus, &CompCode, &Reason);
		switch ( chlStatus ){
			case MQCHS_PAUSED:
				printf("NOTOK\t%s en estado: Paused", chlName);
				controlChannel++;
				break;
			case MQCHS_RETRYING:
				printf("NOTOK\t%s en estado: Retrying", chlName);
				controlChannel++;
				break;
			case MQCHS_STOPPED:
				printf("NOTOK\t%s en estado: Stopped", chlName);
				controlChannel++;
				break;
			default:
				continue;
		}
			mqInquireInteger(cAttrsBag, MQIACH_CHANNEL_TYPE, MQIND_NONE, &chlType, &CompCode, &Reason);
		switch ( chlType ){
			case MQCHT_SENDER:
				printf(" de tipo: Sender\n");
				break;
			case MQCHT_RECEIVER:
				printf(" de tipo: Receiver\n");
				break;
			case MQCHT_SVRCONN:
				printf(" de tipo: Client\n");
				break;
		}
	}

	mqDeleteBag(&adminBag, &CompCode, &Reason);
	mqDeleteBag(&responseBag, &CompCode, &Reason);

	MQDISC(&Hcon, &CompCode, &Reason);
	if ( controlChannel == 0 ){
		printf("OK\tTodos los canales estan funcionando\n");
	}
	return 0;
}
