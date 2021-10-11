/* USER CODE BEGIN Header */

/**
 ******************************************************************************
 *	File		:	main.c
 *	Brief		:	C source file of ThreadX demo application.
 *	Created on	:	Sep 17, 2021
 *	Author		:	William, An.
 *	Email		:	ponytail2k@gmail.com
 ******************************************************************************
 *
 * Copyright (c) 2021 Lee & An Partners Co., Ltd. All rights reserved.
 *
 * This software component is licensed by Lee & An Partners under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {                              // Message object structure
	int	Voltage;                              // AD result of measured voltage
	int	Current;                              // AD result of measured current
	int	Counter;                              // A counter value
} sMeasure;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define	SIGNAL_BIT		0x00000001
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ETH_HandleTypeDef heth;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
UART_HandleTypeDef *console = &huart3;

// Define the ThreadX object control blocks...
uint32_t  OneShotTimerArg;									// argument for the timer call back function
uint32_t  PeriodicTimerArg;									// argument for the timer call back function

osTimerId		OneShotTmrId;								// OneShotTimer id
osTimerId		PeriodicTmrId;								// PeriodicTimer id

osMutexId		MutexId;
osSemaphoreId	SemaphoreId;								// Semaphore ID
osPoolId  		MemPoolId;
osMessageQId	MsgQId;
osMessageQId	PushButtonMsgQID;
osMailQId		MailQId;

osThreadId txMainId;
osThreadId Thread0Id;
osThreadId Thread1Id;
osThreadId Thread2Id;
osThreadId Thread3Id;
osThreadId Thread4Id;
osThreadId Thread5Id;
osThreadId Thread6Id;
osThreadId Thread7Id;
osThreadId Thread8Id;
osThreadId Thread9Id;

// Define the counters used in the demo application...
ULONG thread_0_counter;
ULONG thread_1_counter;
ULONG thread_1_messages_sent;
ULONG thread_2_counter;
ULONG thread_2_messages_received;
ULONG thread_3_counter;
ULONG thread_4_counter;
ULONG thread_5_counter;
ULONG thread_6_counter;
ULONG thread_7_counter;
ULONG thread_8_counter;
ULONG thread_8_mail_sent;
ULONG thread_9_counter;
ULONG thread_9_mail_received;

// prototypes for timer callback function
void OneShotTimerCallback(ULONG timer_input);
void PeriodicTimerCallback(ULONG timer_input);


// Define thread prototypes.
void txMainThread(ULONG thread_input);
void AdcThread(ULONG thread_input);
void KeyThread(ULONG thread_input);
void LedThread(ULONG thread_input);
void Thread0(ULONG thread_input);
void Thread1(ULONG thread_input);
void Thread2(ULONG thread_input);
void Thread34(ULONG thread_input);
void Thread5(ULONG thread_input);
void Thread67(ULONG thread_input);
void Thread8(ULONG thread_input);
void Thread9(ULONG thread_input);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_ETH_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void EnterInterrupt(void)
{
	HAL_GPIO_WritePin(IntGen_GPIO_Port, IntGen_Pin, GPIO_PIN_RESET);
}

void ExitInterrupt(void)
{
	HAL_GPIO_WritePin(IntGen_GPIO_Port, IntGen_Pin, GPIO_PIN_SET);
}
void PushButtonRelease(void)
{
	HAL_GPIO_WritePin(IntGen_GPIO_Port, IntGen_Pin, GPIO_PIN_RESET);
}

void PushButtonPress(void)
{
	HAL_GPIO_WritePin(IntGen_GPIO_Port, IntGen_Pin, GPIO_PIN_SET);
}

// Declare the tx_application_define function as having C linkage.
VOID tx_application_define(VOID *first_unused_memory)
{
    // Related to ThreadX RTOS layer.
    {
    	// Create a byte memory pool from which to allocate the thread stacks.
    	//UINT  _tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr, VOID *pool_start, ULONG pool_size)
    	if (tx_byte_pool_create(&sTxBytePool, "BytePool", first_unused_memory, TX_SYSTEM_BYTE_POOL_SIZE) == TX_SUCCESS) {
    		ULONG	avail = 0;

    		//UINT  _tx_byte_pool_info_get(TX_BYTE_POOL *pool_ptr, CHAR **name, ULONG *available_bytes,
            //								ULONG *fragments, TX_THREAD **first_suspended,
            //								ULONG *suspended_count, TX_BYTE_POOL **next_pool)
    		tx_byte_pool_info_get(&sTxBytePool, TX_NULL, &avail, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
    		kprintf("Thread X BytePool initialization success! (%d)\r\n", avail);
    	} else {
    		kprintf("Thread X BytePool initialization failed!\r\n");
    		while (1) {
    			;
    		}
    	}
    }


	// Put system definition stuff in here, e.g. thread creates and other assorted create information.
    // Related to CMSIS-RTOS layer.
    {
    	//  Create software timer
    	osTimerDef(OneShotTimer, OneShotTimerCallback, 1000);                      // define timers
    	OneShotTimerArg = 1;
    	OneShotTmrId = osTimerCreate (osTimer(OneShotTimer), osTimerOnce, (void *)OneShotTimerArg);

    	osTimerDef(PeriodicTimer, PeriodicTimerCallback, 10000);
    	PeriodicTimerArg = 2;
    	PeriodicTmrId = osTimerCreate (osTimer(PeriodicTimer), osTimerPeriodic, (void *)PeriodicTimerArg);

    	//	Create memory pool
    	osPoolDef(MemPool, 16, sMeasure);                    // Define memory pool
    	MemPoolId = osPoolCreate(osPool(MemPool));

    	//	Create msg queue
    	osMessageQDef(MsgQ, 16, sMeasure*);              // Define message queue
    	MsgQId = osMessageCreate(osMessageQ(MsgQ), NULL);

    	//  Create Push Button Message Key
    	osMessageQDef(PushButtonMsgQ, NUM_OF_KEY_MSGQ, uint32_t);              // Define message queue
    	PushButtonMsgQID = osMessageCreate(osMessageQ(PushButtonMsgQ), NULL);

    	//  Create semaphore
    	osSemaphoreDef(Semaphore);                       // Semaphore definition
    	SemaphoreId = osSemaphoreCreate(osSemaphore(Semaphore), 1);

    	//  Create mutex
    	osMutexDef(Mutex);
    	MutexId = osMutexCreate(osMutex(Mutex));

    	osMailQDef(MailQ, 16, sMeasure);                    // Define mail queue
    	MailQId = osMailCreate(osMailQ(MailQ), NULL);      // create mail queue

    	//	Create Main thread
    	osThreadDef(txMain, txMainThread, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	txMainId = osThreadCreate(osThread(txMain), (void *)0);

    	//	Create PushButton thread
    	osThreadDef(txPushButton, KeyThread, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	txMainId = osThreadCreate(osThread(txPushButton), (void *)0);

    	//	Create Thread 0
    	osThreadDef(Thread0, Thread0, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread0Id = osThreadCreate(osThread(Thread0), (void *)0);

    	// Create Thread 1
    	osThreadDef(Thread1, Thread1, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread1Id = osThreadCreate(osThread(Thread1), (void *)1);

    	// Create Thread 2
    	osThreadDef(Thread2, Thread2, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread2Id = osThreadCreate(osThread(Thread2), (void *)2);

    	// Create Thread 3
    	osThreadDef(Thread3, Thread34, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread3Id = osThreadCreate(osThread(Thread3), (void *)3);

    	// Create Thread 4
    	osThreadDef(Thread4, Thread34, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread4Id = osThreadCreate(osThread(Thread4), (void *)4);

    	// Create Thread 5
    	osThreadDef(Thread5, Thread5, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread5Id = osThreadCreate(osThread(Thread5), (void *)5);

    	// Create Thread 6
    	osThreadDef(Thread6, Thread67, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread6Id = osThreadCreate(osThread(Thread6), (void *)6);

    	// Create Thread 7
    	osThreadDef(Thread7, Thread67, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread7Id = osThreadCreate(osThread(Thread7), (void *)7);

    	// Create Thread 8
    	osThreadDef(Thread8, Thread8, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread8Id = osThreadCreate(osThread(Thread8), (void *)8);

    	// Create Thread 9
    	osThreadDef(Thread9, Thread9, TX_THREAD_STACK_SIZE, osPriorityNormal);
    	Thread9Id = osThreadCreate(osThread(Thread9), (void *)9);
    }
}


/* Define the txMainThread threads.  */
void txMainThread(ULONG thread_input)
{
	osTimerStart(OneShotTmrId, TX_ZERO);                // start timer
	osTimerStart(PeriodicTmrId, TX_ZERO);                // start timer

	/* This thread simply sits in while-forever-sleep loop.  */
	while (1) {
		kprintf("Hello, ThreadX!\r\n");
		osDelay(1000);
		PushButtonPress();	// Single Short Key
		osDelay(90);
		PushButtonRelease();
		osDelay(110);
		PushButtonPress();
		osDelay(90);
		PushButtonRelease();
		osDelay(110);
		PushButtonPress();
		osDelay(90);
		PushButtonRelease();
		osDelay(110);
		PushButtonPress();
		osDelay(90);
		PushButtonRelease();
		osDelay(110);
		PushButtonPress();		// Multi Short Key 1
		osDelay(90);
		PushButtonRelease();
		osDelay(110);

		PushButtonPress();
		osDelay(90);
		PushButtonRelease();
		osDelay(110);
		PushButtonPress();		// Multi Short Key 2
		osDelay(90);
		PushButtonRelease();
		osDelay(110);

		osDelay(1000);

		PushButtonPress();		// Single Long Key
		osDelay(2200);
		PushButtonRelease();
		osDelay(300);
	}
}

void OneShotTimerCallback(ULONG timer_input)
{                   										// timer callback function
	kprintf("OST(%d)\r\n", timer_input);							// arg contains &exec
}

void PeriodicTimerCallback(ULONG timer_input)
{
	kprintf("PT(%d)\r\n", timer_input);							// arg contains &exec
    // called every 10 seconds after osTimerStart
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Key Thread
///////////////////////////////////////////////////////////////////////////////////////////////
uint8_t KeyPressed = GPIO_PIN_RESET;
uint8_t ucKeyIndex = (uint8_t)TX_ZERO;
uint32_t FirstKeyTick = (uint32_t)TX_ZERO;
uint32_t curKeyTick = (uint32_t)TX_ZERO;
uint32_t KeyPressTick[NUM_OF_KEY_BUFFERS];

void PushButtonInterrupt(uint16_t GPIO_Pin)
{
	curKeyTick = osKernelSysTick();

	if (HAL_GPIO_ReadPin(PushButton_GPIO_Port, GPIO_Pin) & GPIO_PIN_SET) {
		KeyPressed = GPIO_PIN_SET;
		KeyPressTick[ucKeyIndex] = curKeyTick;
		osMessagePut(PushButtonMsgQID, (uint32_t)SingleShortKey, 0);
	} else {
		KeyPressed = GPIO_PIN_RESET;
	}
}

void KeyThread(ULONG argument)
{
	uint32_t KeyEvtInfo;
	uint32_t curKeyTick;
	osEvent KeyEvent;

	memset(&KeyPressTick[0], 0, sizeof(uint32_t)*NUM_OF_KEY_BUFFERS);

	for ( ; ; ) {
		KeyEvent = osMessageGet(PushButtonMsgQID, 100);

		curKeyTick = osKernelSysTick();

        if (KeyEvent.status == osEventMessage) {
        	// Receive Key
        	KeyEvtInfo = KeyEvent.value.v;

        	// Key handle
        	if (KeyEvtInfo == SingleShortKey) {
            	if (ucKeyIndex == 0) {			        						// Check First Single Short Key
            		kprintf("SSK\r\n");
            		FirstKeyTick = KeyPressTick[0];
            	} else if (ucKeyIndex == MULTI_SHORT_KEY1_COUNT) {				// Check Multi Short Key
            		if ((KeyPressTick[MULTI_SHORT_KEY1_COUNT] - FirstKeyTick) < KEY_PROCESSING_TIMEOUT) {
            			kprintf("MSK1\r\n");
            			KeyEvtInfo = (uint32_t)MultiShortKey1;
            		}
            	} else if (ucKeyIndex == MULTI_SHORT_KEY2_COUNT) {
            		if ((KeyPressTick[MULTI_SHORT_KEY2_COUNT] - FirstKeyTick) < KEY_PROCESSING_TIMEOUT) {
            			kprintf("MSK2\r\n");
            			KeyEvtInfo = (uint32_t)MultiShortKey2;
            			ucKeyIndex = 0;
            			FirstKeyTick = 0;
            		}
            	}
            	ucKeyIndex++;
        	}
        } else if (KeyEvent.status == osEventTimeout) {							// Check Single Long Key - Timeout
    		// Handle Single Long Key Timeout
        	if (ucKeyIndex != 0) {	// Key handling state
        		// over 2000ms... : Key input timeout occur!!!
        		if ((curKeyTick - FirstKeyTick) > KEY_PROCESSING_TIMEOUT) {
            		// Still key is pressed,
                	if (((curKeyTick - KeyPressTick[ucKeyIndex-1]) > KEY_PROCESSING_TIMEOUT) && (KeyPressed == 1)) {
                		kprintf("SLK\r\n");
                		KeyEvtInfo = (uint32_t)SingleLongKey;
            			ucKeyIndex = 0;							// Timeout : previous key clear
            			FirstKeyTick = 0;
                	}

                	if (KeyPressed == 0) {
            			ucKeyIndex = 0;							// Timeout : previous key clear
            			FirstKeyTick = 0;
                	}
        		}
        	}
        }
	}
}


void Thread0(ULONG thread_input)
{
	UINT status;

	// This thread simply sits in while-forever-sleep loop.
	while (1) {
		// Increment the thread counter.
		thread_0_counter++;

		// Sleep for 10 ticks.
		//tx_thread_sleep(10);
		osDelay(1000);

		// Set event flag 0 to wakeup thread 5.
		//status = tx_event_flags_set(&event_flags_0, 0x1, TX_OR);
		status = osSignalSet(Thread5Id, SIGNAL_BIT);

		// Check status.
		if (status == 0x80000000) {
			break;
		} else {
			kprintf("T0:%08X\r\n", status);
		}
	}
}


void Thread1(ULONG thread_input)
{
	osStatus status;
	sMeasure *pSndMeasure;

    // This thread simply sends messages to a queue shared by thread 2.
    while(1)
    {
        // Increment the thread counter.
        thread_1_counter++;

        // Send message to queue 0.
        //status = tx_queue_send(&queue_0, &thread_1_messages_sent, TX_WAIT_FOREVER);
        pSndMeasure = osPoolCAlloc(MemPoolId);                     // Allocate memory for the message
        pSndMeasure->Voltage = thread_1_messages_sent;                        // Set the message content
        pSndMeasure->Current = thread_1_messages_sent;
        pSndMeasure->Counter = thread_1_counter;

        status = osMessagePut(MsgQId, (uint32_t)pSndMeasure, osWaitForever);  // Send Message

        // Check completion status.
        if (status == osOK) {
            kprintf("T1:%d\r\n", pSndMeasure->Counter);
        } else {
            kprintf("T1E:%d\r\n", status);
            break;
        }

        osDelay(2000);

        // Increment the message sent.
        thread_1_messages_sent++;
    }
}


void Thread2(ULONG thread_input)
{
    //UINT status;
    sMeasure *pRcvMeasure;
    osEvent  th2Event;

    // This thread retrieves messages placed on the queue by thread 1.
    while(1)
    {
        // Increment the thread counter.
        thread_2_counter++;

        // Retrieve a message from the queue.
        //status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);
        th2Event = osMessageGet(MsgQId, osWaitForever);  // wait for message

        // Check completion status and make sure the message is what we expected.
        //if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
        //    break;
        if (th2Event.status == osEventMessage) {
        	pRcvMeasure = th2Event.value.p;
        	kprintf("T2:%d\r\n", pRcvMeasure->Counter);
        	osPoolFree(MemPoolId, pRcvMeasure);                  // free memory allocated for message
        }

        // Otherwise, all is okay. Increment the received message count.
        thread_2_messages_received++;
    }
}


void Thread34(ULONG thread_input)
{
    UINT status;

    // This function is executed from thread 3 and thread 4. As the loop
    //   below shows, these function compete for ownership of semaphore_0.
    while(1)
    {
        // Increment the thread counter.
        if (thread_input == 3) {
            thread_3_counter++;
        } else {
            thread_4_counter++;
        }

        // Get the semaphore with suspension.
        //status = tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);
        status = osSemaphoreWait(SemaphoreId, TX_WAIT_FOREVER);

        if (thread_input == 3) {
            kprintf("T%d(%d)\r\n", thread_input, thread_3_counter);
        } else {
            kprintf("T%d(%d)\r\n", thread_input, thread_4_counter);
        }

        // Check status.
        if (status != TX_SUCCESS)
            break;

        // Sleep for 2 ticks to hold the semaphore.
        //tx_thread_sleep(2);
        osDelay(3000);

        // Release the semaphore.
        //status = tx_semaphore_put(&semaphore_0);
        osSemaphoreRelease(SemaphoreId);

        // Check status.
        //if (status != TX_SUCCESS)
        //    break;
    }
}


void Thread5(ULONG thread_input)
{
	osEvent event;
    //ULONG actual_flags;

    // This thread simply waits for an event in a forever loop.
    while(1)
    {
        // Increment the thread counter.
        thread_5_counter++;

        // Wait for event flag 0.
        //status = tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
        event = osSignalWait(SIGNAL_BIT, osWaitForever);

        // Check status.
        //if ((status != TX_SUCCESS) || (actual_flags != 0x1))
        if (event.status != osEventSignal) {
    		kprintf("T5E:%08X\r\n", event.status);
            break;
        } else {
        	if (event.value.signals == SIGNAL_BIT) {
        		kprintf("T5S:%08X\r\n", event.value.signals);
            } else {
            	kprintf("T5E:%08X\r\n", event.value.signals);
            	break;
            }
        }
    }
}


void Thread67(ULONG thread_input)
{
    //UINT status;

    // This function is executed from thread 6 and thread 7. As the loop
    //    below shows, these function compete for ownership of mutex_0.
    while(1)
    {
        // Increment the thread counter.
        if (thread_input == 6)
            thread_6_counter++;
        else
            thread_7_counter++;

        // Get the mutex with suspension.
        //status = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);
        osMutexWait(MutexId, osWaitForever);

        // Check status.
        //if (status != TX_SUCCESS)
        //    break;

        // Get the mutex again with suspension. This shows
        //    that an owning thread may retrieve the mutex it
        //    owns multiple times.
        //status = tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);
        osMutexWait(MutexId, osWaitForever);

        // Check status.
        //if (status != TX_SUCCESS)
        //    break;

        if (thread_input == 6) {
            kprintf("T%d(%d)\r\n", thread_input, thread_6_counter);
        } else {
            kprintf("T%d(%d)\r\n", thread_input, thread_7_counter);
        }

        // Sleep for 2 ticks to hold the mutex.
        //tx_thread_sleep(2);
        osDelay(4000);

        // Release the mutex.
        //status = tx_mutex_put(&mutex_0);
        osMutexRelease(MutexId);

        // Check status.
        //if (status != TX_SUCCESS)
        //    break;

        // Release the mutex again. This will actually
        //    release ownership since it was obtained twice.
        //status = tx_mutex_put(&mutex_0);
        osMutexRelease(MutexId);

        // Check status.
        //if (status != TX_SUCCESS)
        //    break;
    }
}


void Thread8(ULONG thread_input)
{
	osStatus status;
	sMeasure *pSndMeasure;

    // This thread simply sends messages to a queue shared by thread 2.
    while(1)
    {
        // Increment the thread counter.
        thread_8_counter++;

        // Send message to queue 0.
        //status = tx_queue_send(&queue_0, &thread_1_messages_sent, TX_WAIT_FOREVER);
        pSndMeasure = osMailAlloc(MailQId, osWaitForever);       // Allocate memory
        pSndMeasure->Voltage = thread_8_mail_sent;                        // Set the message content
        pSndMeasure->Current = thread_8_mail_sent;
        pSndMeasure->Counter = thread_8_counter;

        status = osMailPut(MailQId, pSndMeasure);                         // Send Mail

        // Check completion status.
        if (status == osOK) {
            kprintf("T8:%d\r\n", pSndMeasure->Counter);
        } else {
            kprintf("T8E:%d\r\n", status);
            break;
        }

        osDelay(5000);

        // Increment the message sent.
        thread_8_mail_sent++;
    }
}


void Thread9(ULONG thread_input)
{
    //UINT status;
    sMeasure *pRcvMeasure;
    osEvent  th9Event;

    // This thread retrieves messages placed on the queue by thread 1.
    while(1)
    {
        // Increment the thread counter.
        thread_9_counter++;

        // Retrieve a message from the queue.
        //status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);
        th9Event = osMailGet(MailQId, osWaitForever);        // wait for mail

        // Check completion status and make sure the message is what we expected.
        //if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
        //    break;
        if (th9Event.status == osEventMail) {
        	pRcvMeasure = th9Event.value.p;
        	kprintf("T9:%d\r\n", pRcvMeasure->Counter);
        	osMailFree(MailQId, pRcvMeasure);                    // free memory allocated for mail
        }

        // Otherwise, all is okay. Increment the received message count.
        thread_9_mail_received++;
    }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  // Initialization HAL Tick count
  uwTick = 0;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_ETH_Init();
  /* USER CODE BEGIN 2 */

  kprintf("%s %s %s\r\n", osKernelSystemId, __DATE__, __TIME__);

  osKernelInitialize();

  osKernelStart();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  heth.Init.AutoNegotiation = ETH_AUTONEGOTIATION_ENABLE;
  heth.Init.Speed = ETH_SPEED_100M;
  heth.Init.DuplexMode = ETH_MODE_FULLDUPLEX;
  heth.Init.PhyAddress = DP83848_PHY_ADDRESS;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x17;
  MACAddr[2] = 0xA1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x01;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.RxMode = ETH_RXINTERRUPT_MODE;
  heth.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  heth.Init.MediaInterface = ETH_MEDIA_INTERFACE_RMII;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, OrangeLED_Pin|GreenLED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(IntGen_GPIO_Port, IntGen_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PushButton_Pin */
  GPIO_InitStruct.Pin = PushButton_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PushButton_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OrangeLED_Pin GreenLED_Pin */
  GPIO_InitStruct.Pin = OrangeLED_Pin|GreenLED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : IntGen_Pin */
  GPIO_InitStruct.Pin = IntGen_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(IntGen_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : EINT7_Pin */
  GPIO_InitStruct.Pin = EINT7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EINT7_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
