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

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;					// queue message
	osStatus_t retCode;									// return error code
	
	uint8_t * qPtr;
	
	uint8_t srcAddr = 0;
	uint8_t srcSAPI = 0;
	uint8_t dstAddr = 0;
	uint8_t dstSAPI = 0;
	
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
			srcAddr = qPtr[1]>>3;
			srcSAPI = qPtr[1]&0x07;
			dstAddr = qPtr[2]>>3;
			dstSAPI = qPtr[2]&0x07;
			
			// if destination is us
			if(dstAddr == MYADDRESS){
				// => check sapi => redirect
				switch(dstSAPI){
					case CHAT_SAPI:
						
						break;
					case TIME_SAPI:
						break;
					default:
						break;
				};
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


