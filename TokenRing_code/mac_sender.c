//////////////////////////////////////////////////////////////////////////////////
/// \file mac_sender.c
/// \brief MAC sender thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_led.h"

static void sendToPhy (uint8_t * msg);
uint8_t SAPI (void);
void majTokenList(	uint8_t * msg);
void assignQueue(struct queueMsg_t* receiver,struct queueMsg_t* sender);


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	uint8_t * msg;
	struct queueMsg_t queueMsg;					// queue message
	struct queueMsg_t tokenMsg;					// queue message
	osStatus_t retCode;									// return error code
	uint8_t * qPtr;
	uint8_t length;

	uint8_t srcAddr = 0;
	uint8_t srcSAPI = 0;
	uint8_t dstAddr = 0;
	uint8_t dstSAPI = 0;

	uint8_t sum = 0;

	gTokenInterface.connected = 1;
		//------------------------------------------------------------------------------
	for (;;)														// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet(
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever);
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

		qPtr = queueMsg.anyPtr;

		switch (queueMsg.type){

			case DATA_IND :
				srcAddr = MYADDRESS;
				dstAddr = queueMsg.addr;
				srcSAPI = queueMsg.sapi;
				dstSAPI = queueMsg.sapi;		// consider to send to the same sapi as the src

				// shift tableaux
				length = strlen(qPtr);
				memcpy(&qPtr[3],qPtr,length*sizeof(uint8_t));

				// length
				qPtr[2] = length;

				// control src
				qPtr[0] = ((srcAddr&0x0F)<<3) + (srcSAPI&0x07);

				// control dst
				qPtr[1] = ((dstAddr&0x0F)<<3) + (dstSAPI&0x07);

				// status (checksum)
				for(uint8_t i=0; i<length+3; i++){
						sum += qPtr[i];
				}

				qPtr[length+3] = (sum & 0x3F)<<2;

				// prepare message
				// queueMsg->sapi = 0;
				// queueMsg->addr = 0;

				// save for databack !

				// push To queue
				retCode = osMessageQueuePut(
					queue_macS_b_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);

				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

			break;

			case TOKEN:

				// update SAPI
				qPtr[MYADDRESS+1] = ((gTokenInterface.connected << CHAT_SAPI)|(1<<TIME_SAPI));

				// maj token list if different
				majTokenList(qPtr);

				// save token
				assignQueue(&tokenMsg,&queueMsg);

				// get internal queue
				retCode = osMessageQueueGet(
					queue_macS_b_id,
					&queueMsg,
					NULL,
					0); 	// return instantally

				if(retCode == osOK ){
					// push internal msg

					// mise en forme
					msg = queueMsg.anyPtr;

					// send
					sendToPhy(msg);
				}
				else{
					// redonne le token
					sendToPhy(tokenMsg.anyPtr);
				}


			break;

			case DATABACK:
				
			break;

			case NEW_TOKEN:

				msg = osMemoryPoolAlloc(memPool,osWaitForever);
				msg[0] = TOKEN_TAG;

				for(int i = 1; i<TOKENSIZE-3;i++){
					msg[i] = 0x00;
				}

				sendToPhy(msg);

			break;

			case START:
				gTokenInterface.connected = 1;
			break;

			case STOP:
				gTokenInterface.connected = 0;
			break;

			default:

			break;


		};
	}
}

void assignQueue(struct queueMsg_t* receiver,struct queueMsg_t* sender){
	receiver->addr = sender->addr;
	receiver->anyPtr = sender->anyPtr;
	receiver->sapi = sender->sapi;
	receiver->type = sender->type;
}

void majTokenList(	uint8_t * msg){
	uint8_t changed = 0;
	struct queueMsg_t queueMsg;
	osStatus_t retCode;

	//check if change
	if(msg[0] != TOKEN_TAG){
		return;
	}

	for(int i = 1; i <= TOKENSIZE-3;i++){	// 1 to 16
		if(msg[i] != gTokenInterface.station_list[i-1]){
			gTokenInterface.station_list[i-1] = msg[i];
			changed = 1;
		}
	}

	//if change => send TOKEN_LIST to LCD
	if(changed){

		queueMsg.type = TOKEN_LIST;
		queueMsg.anyPtr = NULL;
		queueMsg.addr = NULL;
		queueMsg.sapi = NULL;

		retCode = osMessageQueuePut(
			queue_lcd_id,
			&queueMsg,
			osPriorityNormal,
			osWaitForever);

		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
	}
}


static void sendToPhy (uint8_t * msg){
	struct queueMsg_t queueMsg;
	osStatus_t retCode;

	queueMsg.type = TO_PHY;
	queueMsg.anyPtr = msg;
	queueMsg.addr = NULL;
	queueMsg.sapi = NULL;

	retCode = osMessageQueuePut(
		queue_phyS_id,
		&queueMsg,
		osPriorityNormal,
		osWaitForever);

	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
}
