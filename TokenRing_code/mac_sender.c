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
void macError(char msg[]);


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	char ERROR_MSG[255];

	uint8_t * msg;
	struct queueMsg_t queueMsg;					// queue message
	struct queueMsg_t tokenMsg;					// queue message
	struct queueMsg_t saveMsg;						// queue message
	struct queueMsg_t newMsg;						// queue message
	osStatus_t retCode;									// return error code
	uint8_t * qPtr;
	uint8_t * newqPtr;
	uint8_t length;
	uint8_t status = 0;

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

		if(retCode != osOK){
			continue;
		}

		switch (queueMsg.type){

			case DATA_IND :
				srcAddr = MYADDRESS;
				dstAddr = queueMsg.addr;
				srcSAPI = queueMsg.sapi;
				dstSAPI = queueMsg.sapi;		// consider to send to the same sapi as the src
				length = strlen(qPtr);

				// check if we have to throw away the message (queue_macS_b_id is full) => it prevents to allocate all the memory pool capacity (and block all the system)
				if(osMessageQueueGetSpace(queue_macS_b_id) != 0){
					// alloc memory
					newMsg.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);		// TODO => check if assez place dans la queue macs b

					newqPtr = newMsg.anyPtr;

					// shift tableaux
					memcpy(&newqPtr[3],qPtr,length*sizeof(uint8_t));

					// free old memory
					osMemoryPoolFree(memPool,queueMsg.anyPtr);

					// length
					newqPtr[2] = length;

					// control src
					newqPtr[0] = ((srcAddr&0x0F)<<3) + (srcSAPI&0x07);

					// control dst
					newqPtr[1] = ((dstAddr&0x0F)<<3) + (dstSAPI&0x07);
					sum = 0;
					// status (checksum)
					for(uint8_t i=0; i<length+3; i++){
							sum += newqPtr[i];
					}

					newqPtr[length+3] = (sum & 0x3F)<<2;

					// prepare message
					// queueMsg->sapi = 0;
					// queueMsg->addr = 0;

					// push To queue
					retCode = osMessageQueuePut(
						queue_macS_b_id,
						&newMsg,
						osPriorityNormal,
						osWaitForever);

					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

					#if VERBOSE_MODE == 1
					printf("[DATA_IND] Msg push to internal Queue !\r\n");
					#endif
				}
				else{
					#if VERBOSE_MODE == 1
					printf("[DATA_IND] Msg lost (msg queue full) !\r\n");
					#endif
					// free memory
					osMemoryPoolFree(memPool,queueMsg.anyPtr);
				}
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

					// create a copy !
					saveMsg.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);

					//cpy all
					memcpy(saveMsg.anyPtr,queueMsg.anyPtr,MAX_BLOCK_SIZE*sizeof(uint8_t));

					// send
					sendToPhy(queueMsg.anyPtr);

					#if VERBOSE_MODE == 1
					printf("[TOKEN] send msg, token saved !\r\n");
					#endif
				}
				else{
					// redonne le token
					sendToPhy(tokenMsg.anyPtr);

					#if VERBOSE_MODE == 1
					printf("[TOKEN] return Token !\r\n");
					#endif
				}


			break;

			case DATABACK:
					dstAddr = qPtr[1]>>3;
					length = qPtr[2];
					status = qPtr[3+length];

					if((status & 0x02)>>1 == 1){	// if read bit == 1

						if((status & 0x01) == 1 ){	// if ack bit == 1
							// sunny day
							#if VERBOSE_MODE == 1
							printf("[DATABACK] Msg OK !\r\n");
							#endif

							// free saveMsg if ack is 1 and read is 1
							osMemoryPoolFree(memPool,saveMsg.anyPtr);

							// redonne le token
							sendToPhy(tokenMsg.anyPtr);
						}
						else{
							// probleme de transmission
							#if VERBOSE_MODE == 1
							printf("[DATABACK] ACK error !\r\n");
							#endif
							// resend msg

							// create a copy !
							newMsg.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);

							//cpy all
							memcpy(newMsg.anyPtr,saveMsg.anyPtr,MAX_BLOCK_SIZE*sizeof(uint8_t));

							// send
							sendToPhy(newMsg.anyPtr);
						}
					}
					else{
						#if VERBOSE_MODE == 1
						printf("[DATABACK] READ error !\r\n");
						#endif
						sprintf(ERROR_MSG,"Station %d deconnected\nMessage lost !\n",dstAddr+1);

						macError(ERROR_MSG);

						// redonne le token
						sendToPhy(tokenMsg.anyPtr);
					}

					// free databack msg
					osMemoryPoolFree(memPool,queueMsg.anyPtr);
			break;

			case NEW_TOKEN:
			#if VERBOSE_MODE == 1
				printf("[NEW_TOKEN] created !\r\n");
			#endif
				msg = osMemoryPoolAlloc(memPool,osWaitForever);
				msg[0] = TOKEN_TAG;

				for(int i = 1; i<TOKENSIZE-3;i++){
					msg[i] = 0x00;
				}

				sendToPhy(msg);

			break;

			case START:
				#if VERBOSE_MODE == 1
				printf("[START] connected !\r\n");
				#endif
				gTokenInterface.connected = 1;
			break;

			case STOP:
				#if VERBOSE_MODE == 1
				printf("[STOP] disconnected !\r\n");
				#endif
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

		#if VERBOSE_MODE == 1
		printf("[TOKEN_LIST] update list\r\n");
		#endif

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

void macError(char msg[]){
	struct queueMsg_t queueMsg;
	osStatus_t retCode;

	queueMsg.anyPtr = osMemoryPoolAlloc(memPool,osWaitForever);

	strcpy(queueMsg.anyPtr,msg);

	queueMsg.type = MAC_ERROR;
	queueMsg.addr = MYADDRESS;
	queueMsg.sapi = NULL;

	retCode = osMessageQueuePut(
		queue_lcd_id,
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
