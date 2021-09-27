/**
 ******************************************************************************
 *	File		:	cmsis_os.c
 *	Brief		:	C source file of CMSIS API using ThreadX.
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

/*
 * Copyright (c) 2015, Lab A Part
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this
 * o list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmsis_os.h"

TX_BYTE_POOL			sTxBytePool;
TX_BLOCK_POOL			sTxBlockPool;
TX_EVENT_FLAGS_GROUP	sTxEventFlags;
TX_QUEUE				sTxQueue;
TX_MUTEX				sTxMutex;
TX_SEMAPHORE			sTxSemaphore;

// Convert CMSIS-RTOS priority to ThreadX priority
static UINT ConvertToThreadXPriority(osPriority PriorCMSIS)
{
	return ((UINT)(TX_MAX_PRIORITIES-(((int)PriorCMSIS) + PRIORITY_BIAS)));
}

// Convert ThreadX priority to CMSIS-RTOS priority
static osPriority ConvertToCmsisPriority(UINT PriorThreadX)
{
	return ((osPriority)(TX_MAX_PRIORITIES - ((int)PriorThreadX) - PRIORITY_BIAS));
}

// Thread mode or Handler mode?
#define	inHandlerMode()		(__get_IPSR() != 0)



///////////////////////// Kernel Information and Control /////////////////////////

/// Initialize the RTOS Kernel for creating objects.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osKernelInitialize shall be consistent in every CMSIS-RTOS.
osStatus osKernelInitialize(void)
{
	return osOK;
}

/// Start the RTOS Kernel.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osKernelStart shall be consistent in every CMSIS-RTOS.
osStatus osKernelStart(void)
{
	// VOID        _tx_initialize_kernel_enter(VOID);
	tx_kernel_enter();
	return osOK;
}

/// Check if the RTOS kernel is already started.
/// \note MUST REMAIN UNCHANGED: \b osKernelRunning shall be consistent in every CMSIS-RTOS.
/// \return 0 RTOS is not started, 1 RTOS is started.
int32_t osKernelRunning(void)
{
	UINT ret = 0;

	if (_tx_thread_system_state == TX_INITIALIZE_IS_FINISHED) {
		ret = 1;
	}

	return ret;
}

#if (defined (osFeature_SysTick)  &&  (osFeature_SysTick != 0))

/// Get the RTOS kernel system timer counter
/// \note MUST REMAIN UNCHANGED: \b osKernelSysTick shall be consistent in every CMSIS-RTOS.
/// \return RTOS kernel system timer as 32-bit value
uint32_t osKernelSysTick(void)
{
	return tx_time_get();
}

#endif    // System Timer available



///////////////////////// Thread Management /////////////////////////

/// Create a thread and add it to Active Threads and set it to state READY.
/// \param[in]     thread_def    thread definition referenced with \ref osThread.
/// \param[in]     argument      pointer that is passed to the thread function as start argument.
/// \return thread ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osThreadCreate shall be consistent in every CMSIS-RTOS.
osThreadId osThreadCreate(const osThreadDef_t *thread_def, void *argument)
{
	osThreadId	ThreadID = TX_NULL;
	UINT		RtosPrior;
	CHAR 		*pointer;
	CHAR		*EvtPointer;

	if (inHandlerMode()) {
		ThreadID = TX_NULL;
	} else {
		UINT usStackDepth = TX_MINIMUM_STACK;

		if (thread_def == TX_NULL) {
			ThreadID = TX_NULL;
		} else if ((thread_def->tpriority < osPriorityIdle) || (thread_def->tpriority > osPriorityRealtime)) {
			ThreadID = TX_NULL;
		} else {
			RtosPrior = ConvertToThreadXPriority(thread_def->tpriority);

			if (thread_def->stacksize > usStackDepth) {
				usStackDepth = thread_def->stacksize;
			}

			// Allocate the Thread Control Block for thread.
			//	UINT tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
			if (tx_byte_allocate(&sTxBytePool, (VOID**)&ThreadID, sizeof(TX_THREAD), TX_NO_WAIT) == TX_SUCCESS) {						//	TX_THREAD
				// Allocate the stack for thread.
				//	UINT tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
				if (tx_byte_allocate(&sTxBytePool, (VOID**) &pointer, usStackDepth, TX_NO_WAIT) == TX_SUCCESS) {		//	TX_THREAD_STACK_SIZE
					if (tx_byte_allocate(&sTxBytePool, (VOID**) &EvtPointer, sizeof(TX_EVENT_FLAGS_GROUP), TX_NO_WAIT) == TX_SUCCESS) {
						// Create the main thread.
						// UINT  tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG id), ULONG entry_input,
						//							VOID *stack_start, ULONG stack_size, UINT priority, UINT preempt_threshold,
						//							ULONG time_slice, UINT auto_start)
						if (tx_thread_create(ThreadID, thread_def->name, thread_def->pthread, (ULONG)argument, pointer, usStackDepth, RtosPrior, RtosPrior, TX_NO_TIME_SLICE, TX_AUTO_START) == TX_SUCCESS) {
							ULONG dummy_events;

							// Create the event flags group will be used by thread.
							// UINT  txe_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR *name_ptr, UINT event_control_block_size)
							tx_event_flags_create((TX_EVENT_FLAGS_GROUP *)EvtPointer, thread_def->name);

							// Clear the event flags group in the thread.
							//	UINT tx_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr,
							//							ULONG requested_flags, UINT get_option,
							//							ULONG *actual_flags_ptr, ULONG wait_option)
							tx_event_flags_get((TX_EVENT_FLAGS_GROUP *)EvtPointer, TX_FULL, TX_AND_CLEAR, &dummy_events, TX_NO_WAIT);

							ThreadID->TxEventFlagsGroupPtr = EvtPointer;
						} else {		//	tx_thread_create
							// Release stack memory
							//	UINT  tx_byte_release(VOID *memory_ptr)
							tx_byte_release(EvtPointer);

							// Release stack memory
							//	UINT  tx_byte_release(VOID *memory_ptr)
							tx_byte_release(pointer);

							// Release TCB memory
							//	UINT  tx_byte_release(VOID *memory_ptr)
							tx_byte_release(ThreadID);

							ThreadID = TX_NULL;
						}
					} else {			// tx_byte_allocate for TxEventFlagsGroup (ToDo : )
						// Release stack memory
						//	UINT  tx_byte_release(VOID *memory_ptr)
						tx_byte_release(pointer);

						// Release TCB memory
						//	UINT  tx_byte_release(VOID *memory_ptr)
						tx_byte_release(ThreadID);

						ThreadID = TX_NULL;
					}
				} else {			//	TX_THREAD_STACK_SIZE
					// Release TCB memory
					//	UINT  tx_byte_release(VOID *memory_ptr)
					tx_byte_release(ThreadID);

					ThreadID = TX_NULL;
				}
			}
		}
	}

	return (ThreadID);
}

/// Return the thread ID of the current running thread.
/// \return thread ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osThreadGetId shall be consistent in every CMSIS-RTOS.
osThreadId osThreadGetId(void)
{
	osThreadId ThreadID = NULL;

	if (inHandlerMode()) {
		return (ThreadID);
	} else {
		// TX_THREAD  *_tx_thread_identify(VOID)
		ThreadID = (osThreadId)tx_thread_identify();
	}

	return ThreadID;
}

/// Terminate execution of a thread and remove it from Active Threads.
/// \param[in]     thread_id   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osThreadTerminate shall be consistent in every CMSIS-RTOS.
osStatus osThreadTerminate(osThreadId thread_id)
{
	osStatus ret = osErrorResource;

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		/*
		 * William, An.
		 *
		 * ToDo :	ThreadX's thread can't terminate by itself.
		 *			So this condition is added. (thread_id = tx_thread_identify())
		 */
		//if ((thread_id == TX_NULL) || (thread_id = tx_thread_identify()) || (thread_id->tx_thread_id != TX_THREAD_ID)) {
		if ((thread_id == TX_NULL) || (thread_id->tx_thread_id != TX_THREAD_ID)) {
			ret = osErrorParameter;
		} else {
			osThreadId pThread = thread_id;
			VOID *pStartStk = thread_id->tx_thread_stack_start;
			TX_EVENT_FLAGS_GROUP *pEvtFlagsGroup = thread_id->TxEventFlagsGroupPtr;

			if ((pEvtFlagsGroup == TX_NULL) || (pEvtFlagsGroup->tx_event_flags_group_id != TX_EVENT_FLAGS_ID)) {
				ret = osErrorParameter;
			} else {
				// Remove event flags group
				//	UINT tx_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr)
				if (tx_event_flags_delete(pEvtFlagsGroup) == TX_SUCCESS) {
					// Release EFG (Event Flags Group) memory
					//	UINT  tx_byte_release(VOID *memory_ptr)
					tx_byte_release(pEvtFlagsGroup);
				}

				//	UINT  tx_thread_terminate(TX_THREAD *thread_ptr)
				if (tx_thread_terminate(pThread) == TX_SUCCESS) {
					//	UINT  tx_thread_delete(TX_THREAD *thread_ptr)
					if (tx_thread_delete(pThread) == TX_SUCCESS) {
						// Release stack memory
						//	UINT  tx_byte_release(VOID *memory_ptr)
						tx_byte_release(pStartStk);

						// Release TCB memory
						//	UINT  tx_byte_release(VOID *memory_ptr)
						tx_byte_release(pThread);

						ret = osOK;
					}
				}
			}
		}
	}

	return ret;
}

/// Pass control to next thread that is in state \b READY.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osThreadYield shall be consistent in every CMSIS-RTOS.
osStatus osThreadYield(void)
{
	osStatus ret = osOK;

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		//	VOID  tx_thread_relinquish(VOID)
		tx_thread_relinquish();
	}

	return ret;
}

/// Change priority of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     priority      new priority value for the thread function.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osThreadSetPriority shall be consistent in every CMSIS-RTOS.
osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority)
{
	osStatus ret = osErrorPriority;

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		if ((priority < osPriorityIdle) || (priority > osPriorityRealtime)) {
			ret = osErrorPriority;
		} else {
			if ((thread_id == TX_NULL) || (thread_id->tx_thread_id != TX_THREAD_ID)) {
				ret = osErrorParameter;
			} else {
				UINT old_priority;

				//	UINT  tx_thread_priority_change(TX_THREAD *thread_ptr, UINT new_priority, UINT *old_priority)
				tx_thread_priority_change(thread_id, ConvertToThreadXPriority(priority), &old_priority);

				ret = osOK;
			}
		}
	}

	return ret;
}

/// Get current priority of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return current priority value of the thread function.
/// \note MUST REMAIN UNCHANGED: \b osThreadGetPriority shall be consistent in every CMSIS-RTOS.
osPriority osThreadGetPriority(osThreadId thread_id)
{
	osPriority	ret = osPriorityError;
	UINT		TxPriority;

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		if ((thread_id == TX_NULL) || (thread_id->tx_thread_id != TX_THREAD_ID)) {
			ret = osErrorParameter;
		} else {
			//UINT  tx_thread_info_get(TX_THREAD *thread_ptr, CHAR **name, UINT *state, ULONG *run_count,
			//							UINT *priority, UINT *preemption_threshold, ULONG *time_slice,
			//							TX_THREAD **next_thread, TX_THREAD **next_suspended_thread)
			tx_thread_info_get(thread_id, TX_NULL, TX_NULL, TX_NULL, &TxPriority, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
			ret = ConvertToCmsisPriority(TxPriority);
		}
	}

	return ret;
}



///////////////////////// Generic Wait Functions /////////////////////////
/// Wait for Timeout (Time Delay).
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue "time delay" value
/// \return status code that indicates the execution status of the function.
osStatus osDelay(uint32_t millisec)
{
	osStatus ret = osErrorOS;

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		// UINT  _tx_thread_sleep(ULONG timer_ticks)
		tx_thread_sleep( millisec );

		ret = osEventTimeout;
	}

	return ret;
}

#if (defined (osFeature_Wait)  &&  (osFeature_Wait != 0))     // Generic Wait available

/// Wait for Signal, Message, Mail, or Timeout.
/// \param[in] millisec          \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out
/// \return event that contains signal, message, or mail information or error code.
/// \note MUST REMAIN UNCHANGED: \b osWait shall be consistent in every CMSIS-RTOS.
osEvent osWait(uint32_t millisec)
{
	osEvent ret;

	ret.status = osErrorOS;

	return ret;
}

#endif  // Generic Wait available



///////////////////////// Timer Management /////////////////////////
/// Create a timer.
/// \param[in]     timer_def     timer object referenced with \ref osTimer.
/// \param[in]     type          osTimerOnce for one-shot or osTimerPeriodic for periodic behavior.
/// \param[in]     argument      argument to the timer call back function.
/// \return timer ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osTimerCreate shall be consistent in every CMSIS-RTOS.
osTimerId osTimerCreate(const osTimerDef_t *timer_def, os_timer_type type, void *argument)
{
	osTimerId TimerID = TX_NULL;

#ifndef TX_NO_TIMER
	ULONG	Periodic = TX_ZERO;		// osTimerOnce (One-shot timer)

	if (inHandlerMode()) {
		TimerID = TX_NULL;
	} else {
		if (timer_def != TX_NULL) {
			if (type == osTimerPeriodic) {
				Periodic = timer_def->periodic;	// osTimerPeriodic (Periodic timer)
			}

			// Allocate the memory for timer.
			//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
			tx_byte_allocate(&sTxBytePool, (VOID**) &TimerID, sizeof(TX_TIMER), TX_NO_WAIT);

			// UINT  tx_timer_create(TX_TIMER *timer_ptr, CHAR *name_ptr,
	        //							VOID (*expiration_function)(ULONG id), ULONG expiration_input,
	        //							ULONG initial_ticks, ULONG reschedule_ticks, UINT auto_activate)
			tx_timer_create(TimerID, TX_NULL, timer_def->ptimer, (ULONG)argument, timer_def->periodic, Periodic, TX_NO_ACTIVATE);
		}
	}
#endif		// TX_NO_TIMER

	return TimerID;
}

/// Start or restart a timer.
/// \param[in]     timer_id      timer ID obtained by \ref osTimerCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue "time delay" value of the timer.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osTimerStart shall be consistent in every CMSIS-RTOS.
osStatus osTimerStart(osTimerId timer_id, uint32_t millisec)
{
	osStatus ret = osErrorOS;

#ifndef TX_NO_TIMER

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		if ((timer_id == TX_NULL) || (timer_id->tx_timer_id != TX_TIMER_ID)) {
			ret = osErrorParameter;
		} else {
			// One-short or Periodic?
			ULONG	PeriodicTmr = TX_ZERO;
			//	UINT  _tx_timer_info_get(TX_TIMER *timer_ptr, CHAR **name, UINT *active, ULONG *remaining_ticks,
            //								ULONG *reschedule_ticks, TX_TIMER **next_timer)
			tx_timer_info_get(timer_id, TX_NULL, TX_NULL, TX_NULL, &PeriodicTmr, TX_NULL);
			if (PeriodicTmr != TX_ZERO) {
				if (PeriodicTmr != millisec) {
					//	UINT  _tx_timer_change(TX_TIMER *timer_ptr, ULONG initial_ticks, ULONG reschedule_ticks)
					tx_timer_change(timer_id, millisec, millisec);
				}
			}

			//	UINT  _tx_timer_activate(TX_TIMER *timer_ptr)
			if (tx_timer_activate(timer_id) == TX_SUCCESS) {
				ret = osOK;
			}
		}
	}

#endif		// TX_NO_TIMER

	return ret;
}

/// Stop the timer.
/// \param[in]     timer_id      timer ID obtained by \ref osTimerCreate.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osTimerStop shall be consistent in every CMSIS-RTOS.
osStatus osTimerStop(osTimerId timer_id)
{
	osStatus ret = osErrorOS;

#ifndef TX_NO_TIMER

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		if ((timer_id == TX_NULL) || (timer_id->tx_timer_id != TX_TIMER_ID)) {
			ret = osErrorParameter;
		} else {
			UINT	ActiveTmr = TX_ZERO;
			//	UINT  _tx_timer_info_get(TX_TIMER *timer_ptr, CHAR **name, UINT *active, ULONG *remaining_ticks,
            //								ULONG *reschedule_ticks, TX_TIMER **next_timer)
			tx_timer_info_get(timer_id, TX_NULL, &ActiveTmr, TX_NULL, TX_NULL, TX_NULL);

			if (ActiveTmr == TX_FALSE) {
				ret = osErrorResource;
			} else {
				//	UINT  _tx_timer_deactivate(TX_TIMER *timer_ptr)
				if (tx_timer_deactivate(timer_id) == TX_SUCCESS) {
					ret = osOK;
				}
			}
		}
	}

#endif		// TX_NO_TIMER

	return ret;
}

/// Delete a timer that was created by \ref osTimerCreate.
/// \param[in]     timer_id      timer ID obtained by \ref osTimerCreate.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osTimerDelete shall be consistent in every CMSIS-RTOS.
osStatus osTimerDelete(osTimerId timer_id)
{
	osStatus ret = osErrorOS;

#ifndef TX_NO_TIMER

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		if ((timer_id == TX_NULL) || (timer_id->tx_timer_id != TX_TIMER_ID)) {
			ret = osErrorParameter;
		} else {
			//	UINT  _tx_timer_delete(TX_TIMER *timer_ptr)
			tx_timer_delete(timer_id);

			//	UINT  _tx_byte_release(VOID *memory_ptr)
			tx_byte_release(timer_id);

			ret = osOK;
		}
	}

#endif		// TX_NO_TIMER

	return ret;
}



///////////////////////// Signal Management /////////////////////////

#if (defined (osFeature_Signal)  &&  (osFeature_Signal != 0))     // Signal

/// Set the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signals       specifies the signal flags of the thread that should be set.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
/// \note MUST REMAIN UNCHANGED: \b osSignalSet shall be consistent in every CMSIS-RTOS.
int32_t osSignalSet(osThreadId thread_id, int32_t signals)
{
	ULONG ret = 0x80000000;

	if ((thread_id != NULL) && (thread_id->TxEventFlagsGroupPtr != TX_NULL) && (signals != 0)) {
		//	UINT tx_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr,
		//									CHAR **name, ULONG *current_flags,
		//									TX_THREAD **first_suspended,
		//									ULONG *suspended_count,
		//									TX_EVENT_FLAGS_GROUP **next_group)
		tx_event_flags_info_get(thread_id->TxEventFlagsGroupPtr, TX_NULL, &ret, TX_NULL, TX_NULL, TX_NULL);

		//	UINT tx_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr,
		//							ULONG flags_to_set,UINT set_option)
		tx_event_flags_set(thread_id->TxEventFlagsGroupPtr, signals, TX_OR);
	}

	return ((int32_t)ret);
}

/// Clear the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signals       specifies the signal flags of the thread that shall be cleared.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters or call from ISR.
/// \note MUST REMAIN UNCHANGED: \b osSignalClear shall be consistent in every CMSIS-RTOS.
int32_t osSignalClear(osThreadId thread_id, int32_t signals)
{
	ULONG ret = 0x80000000;

	if ((thread_id != NULL) && (thread_id->TxEventFlagsGroupPtr != TX_NULL) && (signals != 0)) {
		//	UINT tx_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr,
		//									CHAR **name, ULONG *current_flags,
		//									TX_THREAD **first_suspended,
		//									ULONG *suspended_count,
		//									TX_EVENT_FLAGS_GROUP **next_group)
		tx_event_flags_info_get(thread_id->TxEventFlagsGroupPtr, TX_NULL, &ret, TX_NULL, TX_NULL, TX_NULL);

		//	UINT tx_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr,
		//							ULONG flags_to_set,UINT set_option)
		tx_event_flags_set(thread_id->TxEventFlagsGroupPtr, ~signals, TX_AND);
	}

	return ((int32_t)ret);
}

/// Wait for one or more Signal Flags to become signaled for the current \b RUNNING thread.
/// \param[in]     signals       wait until all specified signal flags set or 0 for any single signal flag.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
/// \return event flag information or error code.
/// \note MUST REMAIN UNCHANGED: \b osSignalWait shall be consistent in every CMSIS-RTOS.
osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
	osEvent event;

	event.status =  osErrorOS;

	if (signals != 0) {
		if (inHandlerMode()) {
			event.status =  osErrorISR;
		} else {
			ULONG actual_events;

			osThreadId ThreadID = osThreadGetId();

			if ((ThreadID->TxEventFlagsGroupPtr != NULL)) {
				//	UINT tx_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr,
				//							ULONG requested_flags, UINT get_option,
				//							ULONG *actual_flags_ptr, ULONG wait_option)
				tx_event_flags_get(ThreadID->TxEventFlagsGroupPtr, signals, TX_AND_CLEAR, &actual_events, millisec);

				if ((actual_events & signals) == signals) {
					event.status = osEventSignal;
					event.value.v = actual_events;
				} else {
					if (millisec == 0) {
						event.status = osOK;
					} else {
						event.status = osEventTimeout;
					}
				}
			}
		}
	} else {
		event.status =  osErrorValue;
	}

	return event;
}

#endif	     // Signal



///////////////////////// Mutex Management /////////////////////////

#if (defined (osFeature_Mutex)  &&  (osFeature_Mutex != 0))     // Mutex available

/// Create and Initialize a Mutex object.
/// \param[in]     mutex_def     mutex definition referenced with \ref osMutex.
/// \return mutex ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osMutexCreate shall be consistent in every CMSIS-RTOS.
osMutexId osMutexCreate(const osMutexDef_t *mutex_def)
{
	osMutexId MutexId = NULL;

	if (inHandlerMode()) {
		MutexId = NULL;
	} else {
		if (mutex_def != NULL) {
			// Allocate the memory for mutex.
			//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
			if (tx_byte_allocate(&sTxBytePool, (VOID**) &MutexId, sizeof(TX_MUTEX), TX_NO_WAIT) == TX_SUCCESS) {
				// UINT  tx_mutex_create(TX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit)
				tx_mutex_create(MutexId, TX_NULL, TX_NO_INHERIT);
			}
		}
	}

	return (MutexId);
}

/// Wait until a Mutex becomes available.
/// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osMutexWait shall be consistent in every CMSIS-RTOS.
osStatus osMutexWait(osMutexId mutex_id, uint32_t millisec)
{
	osStatus ret = osErrorOS;

	if (inHandlerMode()) {
		ret = osErrorISR;
	} else {
		if ((mutex_id == NULL) || (mutex_id->tx_mutex_id != TX_MUTEX_ID)) {
			ret = osErrorParameter;
		} else {
			// UINT  tx_mutex_get(TX_MUTEX *mutex_ptr, ULONG wait_option)
			if (tx_mutex_get(mutex_id, millisec) == TX_SUCCESS) {
				ret = osOK;
			} else {
				if (millisec) {
					ret = osErrorTimeoutResource;
				} else {
					ret = osErrorResource;
				}
			}
		}
	}

	return ret;
}

/// Release a Mutex that was obtained by \ref osMutexWait.
/// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osMutexRelease shall be consistent in every CMSIS-RTOS.
osStatus osMutexRelease(osMutexId mutex_id)
{
	osStatus ret = osErrorResource;

	if (inHandlerMode()) {
		ret =  osErrorISR;
	} else {
		if ((mutex_id == NULL) || (mutex_id->tx_mutex_id != TX_MUTEX_ID)) {
			ret = osErrorParameter;
		} else {
			// UINT  tx_mutex_put(TX_MUTEX *mutex_ptr)
			if (tx_mutex_put(mutex_id) == TX_SUCCESS) {
				ret = osOK;
			}
		}
	}

	return ret;
}

/// Delete a Mutex that was created by \ref osMutexCreate.
/// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osMutexDelete shall be consistent in every CMSIS-RTOS.
osStatus osMutexDelete(osMutexId mutex_id)
{
	osStatus ret = osErrorOS;

	if (inHandlerMode()) {
		ret =  osErrorISR;
	} else {
		if ((mutex_id == NULL) || (mutex_id->tx_mutex_id != TX_MUTEX_ID)) {
			ret = osErrorParameter;
		} else {
			// UINT  tx_mutex_delete(TX_MUTEX *mutex_ptr)
			if (tx_mutex_delete(mutex_id) == TX_SUCCESS) {
				//	UINT  tx_byte_release(VOID *memory_ptr)
				tx_byte_release(mutex_id);

				ret = osOK;
			}
		}
	}

	return ret;
}

#endif     // Mutex available



///////////////////////// Semaphore Management Functions /////////////////////////

#if (defined (osFeature_Semaphore)  &&  (osFeature_Semaphore != 0))     // Semaphore available

/// Create and Initialize a Semaphore object used for managing resources.
/// \param[in]     semaphore_def semaphore definition referenced with \ref osSemaphore.
/// \param[in]     count         number of available resources.
/// \return semaphore ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreCreate shall be consistent in every CMSIS-RTOS.
osSemaphoreId osSemaphoreCreate(const osSemaphoreDef_t *semaphore_def, int32_t count)
{
	osSemaphoreId SemaphoreID = TX_NULL;

	if (inHandlerMode()) {
		SemaphoreID = TX_NULL;
	} else {
		if (semaphore_def != TX_NULL) {
			// Allocate the memory for semaphore.
			//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
			if (tx_byte_allocate(&sTxBytePool, (VOID**) &SemaphoreID, sizeof(TX_SEMAPHORE), TX_NO_WAIT) == TX_SUCCESS) {
				// 	UINT tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count)
				tx_semaphore_create(SemaphoreID, TX_NULL, count);
			}
		}
	}

	return (SemaphoreID);
}

/// Wait until a Semaphore token becomes available.
/// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphoreCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
/// \return number of available tokens, or -1 in case of incorrect parameters.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreWait shall be consistent in every CMSIS-RTOS.
int32_t osSemaphoreWait(osSemaphoreId semaphore_id, uint32_t millisec)
{
	int32_t ret = -1;

	if (inHandlerMode()) {
		ret = -1;
	} else {
		if ((semaphore_id == NULL) || (semaphore_id->tx_semaphore_id != TX_SEMAPHORE_ID)) {
			ret = -1;
		} else {
			//	UINT  _tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option)
			if (tx_semaphore_get(semaphore_id, millisec) == TX_SUCCESS) {
				// UINT tx_semaphore_info_get(TX_SEMAPHORE *semaphore_ptr,
				//								CHAR **name, ULONG *current_value,
				//								TX_THREAD **first_suspended,
				//								ULONG *suspended_count,
				//								TX_SEMAPHORE **next_semaphore)
				tx_semaphore_info_get(semaphore_id, TX_NULL, (ULONG *)&ret, TX_NULL, TX_NULL, TX_NULL);
			} else {
				ret = 0;	// Has no available semaphore tokens. (Timeout)
			}
		}
	}

	return ((int32_t)ret);
}

/// Release a Semaphore token.
/// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphoreCreate.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreRelease shall be consistent in every CMSIS-RTOS.
osStatus osSemaphoreRelease(osSemaphoreId semaphore_id)
{
	osStatus ret = osErrorOS;

	if ((semaphore_id == NULL) || (semaphore_id->tx_semaphore_id != TX_SEMAPHORE_ID)) {
		ret = osErrorParameter;
	} else {
		//	UINT tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr)
		if (tx_semaphore_put(semaphore_id) == TX_SUCCESS) {
			ret = osOK;
		} else {
			ret = osErrorResource;
		}
	}

	return ret;
}

/// Delete a Semaphore that was created by \ref osSemaphoreCreate.
/// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphoreCreate.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreDelete shall be consistent in every CMSIS-RTOS.
osStatus osSemaphoreDelete(osSemaphoreId semaphore_id)
{
	osStatus ret = osErrorResource;

	if (inHandlerMode()) {
		ret =  osErrorISR;
	} else {
		if ((semaphore_id == NULL) || (semaphore_id->tx_semaphore_id != TX_SEMAPHORE_ID)) {
			ret = osErrorParameter;
		} else {
			//	UINT tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr)
			tx_semaphore_delete(semaphore_id);

			//	UINT  tx_byte_release(VOID *memory_ptr)
			tx_byte_release(semaphore_id);

			ret = osOK;
		}
	}

	return ret;
}

#endif     // Semaphore available



///////////////////////// Memory Pool Management Functions /////////////////////////
#if (defined (osFeature_Pool)  &&  (osFeature_Pool != 0))		// osFeature_Pool
/*
typedef struct os_pool_cb {
	void 	*pool;
	uint32_t item_sz;
} os_pool_cb_t;
*/
/// Create and Initialize a memory pool.
/// \param[in]     pool_def      memory pool definition referenced with \ref osPool.
/// \return memory pool ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osPoolCreate shall be consistent in every CMSIS-RTOS.
osPoolId osPoolCreate(const osPoolDef_t *pool_def)
{
	osPoolId MemoryPoolID = TX_NULL;

	if (inHandlerMode()) {
		MemoryPoolID = TX_NULL;
	} else {
		if (pool_def != TX_NULL) {
			CHAR *pointer;
			ULONG total_size_of_pool = (pool_def->item_sz + sizeof(void *)) * pool_def->item_num;

			// Allocate the memory for memory pool.
			//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
			if (tx_byte_allocate(&sTxBytePool, (VOID**) &MemoryPoolID, sizeof(TX_BLOCK_POOL), TX_NO_WAIT) == TX_SUCCESS) {
				// Allocate the memory for a small block pool.
				//	UINT  _tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
				if (tx_byte_allocate(&sTxBytePool, (VOID**) &pointer, total_size_of_pool, TX_NO_WAIT) == TX_SUCCESS) {
					// Create a block memory pool to allocate a message buffer from.
					//	UINT  _tx_block_pool_create(TX_BLOCK_POOL *pool_ptr, CHAR *name_ptr, ULONG block_size,
		            //								VOID *pool_start, ULONG pool_size)
					if (tx_block_pool_create(MemoryPoolID, TX_NULL, pool_def->item_sz, pointer, total_size_of_pool) != TX_SUCCESS) {
						MemoryPoolID = TX_NULL;;
					}
				} else {
					//	UINT  tx_byte_release(VOID *memory_ptr)
					tx_byte_release(MemoryPoolID);

					MemoryPoolID = TX_NULL;;
				}
			} else {
				MemoryPoolID = TX_NULL;;
			}
		}
	}

	return MemoryPoolID;
}

/// Allocate a memory block from a memory pool.
/// \param[in]     pool_id       memory pool ID obtain referenced with \ref osPoolCreate.
/// \return address of the allocated memory block or NULL in case of no memory available.
/// \note MUST REMAIN UNCHANGED: \b osPoolAlloc shall be consistent in every CMSIS-RTOS.
void *osPoolAlloc(osPoolId pool_id)
{
	void *pMemBlock = TX_NULL;

	if ((pool_id != TX_NULL) && (pool_id->tx_block_pool_id == TX_BLOCK_POOL_ID)) {
		//	UINT  _tx_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option)
		tx_block_allocate(pool_id, (VOID **)&pMemBlock, TX_NO_WAIT);
	}

	return pMemBlock;
}

/// Allocate a memory block from a memory pool and set memory block to zero.
/// \param[in]     pool_id       memory pool ID obtain referenced with \ref osPoolCreate.
/// \return address of the allocated memory block or NULL in case of no memory available.
/// \note MUST REMAIN UNCHANGED: \b osPoolCAlloc shall be consistent in every CMSIS-RTOS.
void *osPoolCAlloc(osPoolId pool_id)
{
	void *pMemBlock = TX_NULL;

	if ((pool_id != TX_NULL) && (pool_id->tx_block_pool_id == TX_BLOCK_POOL_ID)) {
		//	UINT  _tx_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option)
		tx_block_allocate(pool_id, (VOID **)&pMemBlock, TX_NO_WAIT);

		if (pMemBlock != TX_NULL) {
			memset(pMemBlock, 0, pool_id->tx_block_pool_block_size);
		}
	}

	return pMemBlock;
}

/// Return an allocated memory block back to a specific memory pool.
/// \param[in]     pool_id       memory pool ID obtain referenced with \ref osPoolCreate.
/// \param[in]     block         address of the allocated memory block that is returned to the memory pool.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osPoolFree shall be consistent in every CMSIS-RTOS.
osStatus osPoolFree(osPoolId pool_id, void *block)
{
	osStatus ret = osErrorOS;

	if ((pool_id == TX_NULL) || (pool_id->tx_block_pool_id != TX_BLOCK_POOL_ID) || (block == TX_NULL)) {
		ret = osErrorParameter;
	} else if (((uint32_t)block > ((uint32_t)pool_id->tx_block_pool_start + pool_id->tx_block_pool_size)) || ((uint32_t)block < (uint32_t)pool_id->tx_block_pool_start)) {
		ret = osErrorValue;
	} else {
		// UINT  tx_block_release(VOID *block_ptr)
		tx_block_release(block);

		ret = osOK;
	}

	return ret;
}

#endif					// osFeature_Pool



///////////////////////// Message Queue Management Functions /////////////////////////
#if (defined (osFeature_MessageQ)  &&  (osFeature_MessageQ != 0))		// osFeature_MessageQ

/// Create and Initialize a Message Queue.
/// \param[in]     queue_def     queue definition referenced with \ref osMessageQ.
/// \param[in]     thread_id     thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId) or NULL.
/// \return message queue ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osMessageCreate shall be consistent in every CMSIS-RTOS.
osMessageQId osMessageCreate(const osMessageQDef_t *queue_def, osThreadId thread_id)
{
	osMessageQId MessageQID = TX_NULL;

	if (inHandlerMode()) {
		MessageQID = TX_NULL;
	}
	else {
		if (queue_def != TX_NULL) {
			ULONG available = TX_ZERO;
			uint8_t *pointer = TX_NULL;
			ULONG total_size_of_queue = TX_ZERO;

			total_size_of_queue = (TX_1_ULONG << 2) * queue_def->item_num;

			tx_byte_pool_info_get(&sTxBytePool, TX_NULL, &available, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
			if (available < (total_size_of_queue + sizeof(TX_QUEUE) + 8 * 2) ) {
				MessageQID = TX_NULL;
			} else {
				// Allocate the memory for Message Queue.
				//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
				if (tx_byte_allocate(&sTxBytePool, (VOID**) &MessageQID, sizeof(TX_QUEUE), TX_NO_WAIT) == TX_SUCCESS) {
					// Allocate the memory for a small block pool.
					//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
					if (tx_byte_allocate(&sTxBytePool, (VOID**) &pointer, total_size_of_queue, TX_NO_WAIT)  == TX_SUCCESS) {
						// Create a block memory pool to allocate a message buffer from.
						//	UINT  tx_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size,
						//                        VOID *queue_start, ULONG queue_size)
						tx_queue_create(MessageQID, TX_NULL, TX_1_ULONG, pointer, total_size_of_queue);
					} else {
						//	UINT  tx_byte_release(VOID *memory_ptr)
						tx_byte_release(MessageQID);

						MessageQID = TX_NULL;
					}
				}
			}
		}
	}

	return MessageQID;
}

/// Put a Message to a Queue.
/// \param[in]     queue_id      message queue ID obtained with \ref osMessageCreate.
/// \param[in]     info          message information.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osMessagePut shall be consistent in every CMSIS-RTOS.
osStatus osMessagePut(osMessageQId queue_id, uint32_t info, uint32_t millisec)
{
	osStatus ret = osErrorNoMemory;

	if ((queue_id == NULL) || (queue_id->tx_queue_id != TX_QUEUE_ID)) {
		ret = osErrorParameter;
	} else {
		if (inHandlerMode()) {
			if (millisec) {
				ret = osErrorParameter;
			} else {
				// UINT tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)
				if (tx_queue_send(queue_id, &info, TX_NO_WAIT) == TX_SUCCESS) {
					ret = osOK;
				} else {
					ret = osErrorResource;
				}
			}
		} else {
			// UINT tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)
			if (tx_queue_send(queue_id, &info, millisec) == TX_SUCCESS) {
				ret = osOK;
			} else {
				if (millisec) {
					ret = osErrorTimeoutResource;
				} else {
					ret = osErrorResource;
				}
			}
		}
	}

	return ret;
}

/// Get a Message or Wait for a Message from a Queue.
/// \param[in]     queue_id      message queue ID obtained with \ref osMessageCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
/// \return event information that includes status code.
/// \note MUST REMAIN UNCHANGED: \b osMessageGet shall be consistent in every CMSIS-RTOS.
osEvent osMessageGet(osMessageQId queue_id, uint32_t millisec)
{
	osEvent event;

	event.status =  osErrorOS;

	if ((queue_id == NULL) || (queue_id->tx_queue_id != TX_QUEUE_ID)) {
		event.status =  osErrorParameter;
	} else {
		if (inHandlerMode()) {
			if (millisec) {
				event.status = osErrorParameter;
			} else {
				//	UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)
				if (tx_queue_receive(queue_id, &event.value.p, TX_NO_WAIT) == TX_SUCCESS) {
					event.status = osEventMessage;
				} else {
					event.status = osOK;
				}
			}
		} else {
			//	UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)
			if (tx_queue_receive(queue_id, &event.value.p, millisec) == TX_SUCCESS) {
				event.status = osEventMessage;
			} else {
				if (millisec) {
					event.status = osEventTimeout;
				} else {
					event.status = osOK;
				}
			}
		}
	}

	return event;
}

#endif					// osFeature_MessageQ



///////////////////////// Mail Queue Management Functions /////////////////////////
#if (defined(osFeature_MailQ) && (osFeature_MailQ != 0))     // Mail Queues available

typedef struct os_mailQ_cb {
	uint32_t 		item_num;		///< number of elements in the queue
	uint32_t		item_sz;	 	///< size of an item
	osPoolId		PoolID;
	osMessageQId	MsgQueueID;
} os_mailQ_cb_t;

/// Create and Initialize mail queue.
/// \param[in]     queue_def     reference to the mail queue definition obtain with \ref osMailQ
/// \param[in]     thread_id     thread ID (obtained by \ref osThreadCreate or \ref osThreadGetId) or NULL.
/// \return mail queue ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osMailCreate shall be consistent in every CMSIS-RTOS.
osMailQId osMailCreate(const osMailQDef_t *mailq_def, osThreadId thread_id)
{
	osMailQId MailQID = TX_NULL;

	if (inHandlerMode()) {
		MailQID = TX_NULL;
	} else {
		osPoolDef_t pool_def		= { mailq_def->item_num, mailq_def->item_sz };
		osMessageQDef_t msgq_def	= { mailq_def->item_num };

		if (mailq_def != TX_NULL) {
			ULONG available = TX_ZERO;
			//	UINT  _tx_byte_pool_info_get(TX_BYTE_POOL *pool_ptr, CHAR **name, ULONG *available_bytes,
	        //									ULONG *fragments, TX_THREAD **first_suspended,
	        //									ULONG *suspended_count, TX_BYTE_POOL **next_pool)
			tx_byte_pool_info_get(&sTxBytePool, TX_NULL, &available, TX_NULL, TX_NULL, TX_NULL, TX_NULL);
			if (available < (sizeof (os_mailQ_cb_t) + sizeof(TX_QUEUE) + sizeof(TX_BLOCK_POOL) + (mailq_def->item_num * sizeof(void *)) + (mailq_def->item_num * (mailq_def->item_sz + sizeof(void *)))) ) {
				MailQID = TX_NULL;
			} else {
				// Allocate the memory for Message Queue.
				//	UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,  ULONG wait_option)
				if (tx_byte_allocate(&sTxBytePool, (VOID**) &MailQID, sizeof(os_mailQ_cb_t), TX_NO_WAIT) == TX_SUCCESS) {
					MailQID->item_num 	= mailq_def->item_num;
					MailQID->item_sz	= mailq_def->item_sz;

					// osPoolId osPoolCreate (const osPoolDef_t *pool_def);
					MailQID->PoolID		= osPoolCreate(&pool_def);
					MailQID->MsgQueueID = osMessageCreate(&msgq_def, TX_NULL);
				}
			}
		}
	}

	return MailQID;
}

/// Allocate a memory block from a mail.
/// \param[in]     queue_id      mail queue ID obtained with \ref osMailCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out
/// \return pointer to memory block that can be filled with mail or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osMailAlloc shall be consistent in every CMSIS-RTOS.
void *osMailAlloc(osMailQId queue_id, uint32_t millisec)
{
	void *pMemBlock = NULL;

	if ((queue_id == NULL) || (queue_id->MsgQueueID->tx_queue_id != TX_QUEUE_ID)) {
		return pMemBlock;
	} else {
		if (inHandlerMode()) {
			if (millisec != 0) {
				return pMemBlock;
			}
		}

		pMemBlock = osPoolAlloc(queue_id->PoolID);
	}

	return pMemBlock;
}


/// Allocate a memory block from a mail and set memory block to zero.
/// \param[in]     queue_id      mail queue ID obtained with \ref osMailCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out
/// \return pointer to memory block that can be filled with mail or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osMailCAlloc shall be consistent in every CMSIS-RTOS.
void *osMailCAlloc (osMailQId queue_id, uint32_t millisec)
{
	void *pMemBlock = NULL;

	if ((queue_id == NULL) || (queue_id->MsgQueueID->tx_queue_id != TX_QUEUE_ID)) {
		return pMemBlock;
	} else {
		if (inHandlerMode()) {
			if (millisec != 0) {
				return pMemBlock;
			}
		}

		pMemBlock = osPoolCAlloc(queue_id->PoolID);
	}

	return pMemBlock;
}

/// Put a mail to a queue.
/// \param[in]     queue_id      mail queue ID obtained with \ref osMailCreate.
/// \param[in]     mail          memory block previously allocated with \ref osMailAlloc or \ref osMailCAlloc.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osMailPut shall be consistent in every CMSIS-RTOS.
osStatus osMailPut(osMailQId queue_id, void *mail)
{
	osStatus ret = osErrorOS;

	if ((queue_id == NULL) || (queue_id->PoolID == NULL) || (queue_id->MsgQueueID == NULL) || (queue_id->PoolID->tx_block_pool_id != TX_BLOCK_POOL_ID) || (queue_id->MsgQueueID->tx_queue_id != TX_QUEUE_ID)) {
		ret = osErrorParameter;
	} else if (mail == NULL) {
		ret = osErrorValue;
	} else if (((uint32_t)mail > ((uint32_t)queue_id->PoolID->tx_block_pool_start + queue_id->PoolID->tx_block_pool_block_size)) || ((uint32_t)mail < (uint32_t)queue_id->PoolID->tx_block_pool_start)) {
		ret = osErrorValue;
	} else {
		// UINT tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)
		if (tx_queue_send(queue_id->MsgQueueID, &mail, TX_NO_WAIT) == TX_SUCCESS) {
			ret = osOK;
		} else {
			ret = osErrorResource;
		}
	}

	return ret;
}

/// Get a mail from a queue.
/// \param[in]     queue_id      mail queue ID obtained with \ref osMailCreate.
/// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out
/// \return event that contains mail information or error code.
/// \note MUST REMAIN UNCHANGED: \b osMailGet shall be consistent in every CMSIS-RTOS.
osEvent osMailGet(osMailQId queue_id, uint32_t millisec)
{
	osEvent event;

	event.status =  osErrorOS;

	if ((queue_id == NULL) || (queue_id->PoolID == NULL) || (queue_id->MsgQueueID == NULL) || (queue_id->PoolID->tx_block_pool_id != TX_BLOCK_POOL_ID) || (queue_id->MsgQueueID->tx_queue_id != TX_QUEUE_ID)) {
		event.status = osErrorParameter;
	} else {
		if (inHandlerMode()) {
			if (millisec) {
				event.status = osErrorParameter;
			} else {
				//	UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)
				if (tx_queue_receive(queue_id->MsgQueueID, &event.value.p, TX_NO_WAIT) == TX_SUCCESS) {
					event.status = osEventMail;
				} else {
					event.status = osOK;
				}
			}
		} else {
			//	UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)
			if (tx_queue_receive(queue_id->MsgQueueID, &event.value.p, millisec) == TX_SUCCESS) {
				event.status = osEventMail;
			} else {
				if (millisec) {
					event.status = osEventTimeout;
				} else {
					event.status = osOK;
				}
			}
		}
	}

	return event;
}

/// Free a memory block from a mail.
/// \param[in]     queue_id      mail queue ID obtained with \ref osMailCreate.
/// \param[in]     mail          pointer to the memory block that was obtained with \ref osMailGet.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osMailFree shall be consistent in every CMSIS-RTOS.
osStatus osMailFree(osMailQId queue_id, void *mail)
{
	osStatus ret = osErrorOS;

	if ((queue_id == NULL) || (queue_id->MsgQueueID->tx_queue_id != TX_QUEUE_ID)) {
		ret = osErrorParameter;
	} else if (mail == NULL) {
		ret = osErrorValue;
	} else if (((uint32_t)mail > ((uint32_t)queue_id->PoolID->tx_block_pool_start + queue_id->PoolID->tx_block_pool_block_size)) || ((uint32_t)mail < (uint32_t)queue_id->PoolID->tx_block_pool_start)) {
		ret = osErrorValue;
	} else {
		ret = osPoolFree(queue_id->PoolID, mail);
	}

	return ret;
}

#endif  // Mail Queues available


///////////////////////// Other Functions /////////////////////////
