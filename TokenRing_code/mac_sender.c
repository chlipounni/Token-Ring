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

void sendToPhy (uint8_t * msg);


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	uint8_t * msg;
	struct queueMsg_t queueMsg;					// queue message
	osStatus_t retCode;									// return error code
	
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
		
		switch (queueMsg.type){
			
			case DATA_IND :

			break;
			
			case TOKEN:
			Ext_LED_PWM(2,100);										// token is in station
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
				
			break;
			
			case STOP:
				
			break;
			
			default:
				
			break;
		
		
		};						
	}	
}


void sendToPhy (uint8_t * msg){
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
	

