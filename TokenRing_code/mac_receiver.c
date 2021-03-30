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

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;					// queue message
	osStatus_t retCode;									// return error code
	
	uint8_t * qPtr;
	
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


