//////////////////////////////////////////////////////////////////////////////////
/// \file mac_receiver.c
/// \brief MAC receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"

void sendToken (uint8_t * msg);
static void sendToPhy (uint8_t * msg);
void sendDataBack (uint8_t * msg,uint8_t srcAddr,uint8_t srcSAPI);
uint8_t checkChecksum(uint8_t* qPtr);

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;					// queue message
	struct queueMsg_t queueMsgToSend;					// queue message
	osStatus_t retCode;									// return error code

	uint8_t * qPtr;

	uint8_t * msg;

	uint8_t srcAddr = 0;
	uint8_t srcSAPI = 0;
	uint8_t dstAddr = 0;
	uint8_t dstSAPI = 0;
	uint8_t length = 0;
	uint8_t status = 0;

		//------------------------------------------------------------------------------
	for (;;)														// loop until doomsday
	{
		//----------------------------------------------------------------------------
		// QUEUE READ
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet(
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever);
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

		qPtr = queueMsg.anyPtr;

		if(qPtr[0] == TOKEN_TAG){
			// this is a token !!!!!!!!
			sendToken(qPtr);
		}
		else{
			srcAddr = qPtr[0]>>3;
			srcSAPI = qPtr[0]&0x07;
			dstAddr = qPtr[1]>>3;
			dstSAPI = qPtr[1]&0x07;
			length = qPtr[2];
			status = qPtr[3+length];

			// if destination is us
			if(dstAddr == MYADDRESS || dstAddr == BROADCAST_ADDRESS){
				// => check sapi => redirect
				

				
				// read bit to 1
				qPtr[3+length] |= 0x02;

				if (checkChecksum(qPtr)){
					// ack bit to 1
					qPtr[3+length] |= 0x01;

					// create new message
					msg = osMemoryPoolAlloc(memPool,osWaitForever);

					memcpy(msg,&qPtr[3],length*sizeof(uint8_t));
					msg[length] = '\0';

					queueMsgToSend.type = DATA_IND;
					queueMsgToSend.anyPtr = msg;
					queueMsgToSend.addr = srcAddr;
					queueMsgToSend.sapi = srcSAPI;

					// send to the good queue
					switch(dstSAPI){
						case CHAT_SAPI:
							retCode = osMessageQueuePut(
								queue_chatR_id,
								&queueMsgToSend,
								osPriorityNormal,
								osWaitForever);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							break;
						case TIME_SAPI:
							retCode = osMessageQueuePut(
								queue_timeR_id,
								&queueMsgToSend,
								osPriorityNormal,
								osWaitForever);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							break;
						default:
							break;
					};
				}

				if(srcAddr == MYADDRESS){	// we are the sender !
					// del msg
					sendDataBack(qPtr,srcAddr,srcSAPI);
				}
				else{
					// send databack to others (via TO_PHY)
					sendToPhy(qPtr);
				}
				

			}
			// elsif src is us
			else if(srcAddr == MYADDRESS){
				// => databack
				sendDataBack(qPtr,srcAddr,srcSAPI);
			}
			else{
				// forward to phy (to another station) not for us
				sendToPhy(qPtr);
			}
		}
	}
}

uint8_t checkChecksum(uint8_t* qPtr){
	uint8_t length = qPtr[2];
	uint8_t status = qPtr[3+length];
	uint8_t checksum = status >>2;
	uint8_t sum = 0;

	for(uint8_t i=0; i<length+3; i++){
		sum += qPtr[i];
	}
	if (checksum == (sum & 0x3F)){
		return 1;
	}else{
		return 0;
	}
}

void sendToken (uint8_t * msg){
	struct queueMsg_t queueMsg;
	osStatus_t retCode;

	queueMsg.type = TOKEN;
	queueMsg.anyPtr = msg;
	queueMsg.addr = NULL;
	queueMsg.sapi = NULL;

	retCode = osMessageQueuePut(
		queue_macS_id,
		&queueMsg,
		osPriorityNormal,
		osWaitForever);

	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
}


void sendDataBack (uint8_t * msg,uint8_t srcAddr,uint8_t srcSAPI){
	struct queueMsg_t queueMsg;
	osStatus_t retCode;

	queueMsg.type = DATABACK;
	queueMsg.anyPtr = msg;
	queueMsg.addr = srcAddr;
	queueMsg.sapi = srcSAPI;

	retCode = osMessageQueuePut(
		queue_macS_id,
		&queueMsg,
		osPriorityNormal,
		osWaitForever);

	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
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
