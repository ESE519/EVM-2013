/******************************************************************************
*  Nano-RK, a real-time operating system for sensor networks.
*  Copyright (C) 2007, Real-Time and Multimedia Lab, Carnegie Mellon University
*  All rights reserved.
*
*  This is the Open Source Version of Nano-RK included as part of a Dual
*  Licensing Model. If you are unsure which license to use please refer to:
*  http://www.nanork.org/nano-RK/wiki/Licensing
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, version 2.0 of the License.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  Contributing Authors (specific to this file):
*  Anthony Rowe
*  Zane Starr
*  Anand Eswaren
*******************************************************************************/

#include <nrk.h>
#include <nrk_task.h>
#include <include.h>
#include <ulib.h>
#include <nrk_timer.h>
#include <nrk_scheduler.h>
#include <nrk_error.h>
#include <nrk_stack_check.h>

//#define TIME_PAD  2
//Serial pc6(USBTX, USBRX);
inline void _nrk_wait_for_scheduler ();

uint8_t nrk_get_high_ready_task_ID ()
{
    return (_head_node->task_ID);
}

void nrk_print_readyQ ()
{
    nrk_queue *ptr;
    ptr = _head_node;
    //nrk_kprintf (PSTR ("nrk_queue: "));
    while (ptr != NULL)
    {
        //printf ("%d ", ptr->task_ID);
        ptr = ptr->Next;
    }
    //nrk_kprintf (PSTR ("\n\r"));
}


void nrk_add_to_readyQ (int8_t task_ID)
{
    nrk_queue *NextNode;
    nrk_queue *CurNode;

    //printf( "nrk_add_to_readyQ %d\n",task_ID );
    // nrk_queue full
    if (_free_node == NULL)
    {
        return;
    }


    NextNode = _head_node;
    CurNode = _free_node;

    if (_head_node != NULL)
    {

        while (NextNode != NULL)
        {
            if (nrk_task_TCB[NextNode->task_ID].elevated_prio_flag)
                if (nrk_task_TCB[NextNode->task_ID].task_prio_ceil <
                        nrk_task_TCB[task_ID].task_prio)
                    break;
            if (nrk_task_TCB[task_ID].elevated_prio_flag)
                if (nrk_task_TCB[NextNode->task_ID].task_prio <
                        nrk_task_TCB[task_ID].task_prio_ceil)
                    break;
            if (nrk_task_TCB[NextNode->task_ID].task_prio <
                    nrk_task_TCB[task_ID].task_prio)
                break;
						//Added by sumukh : Ensure that the added task absolute deadline are in ascending order 
						
						if(nrk_task_TCB[task_ID].absolute_deadline < nrk_task_TCB[NextNode->task_ID].absolute_deadline)
							break;
						
            NextNode = NextNode->Next;
        }


        //      while ((NextNode != NULL) && ((nrk_task_TCB[NextNode->task_ID].task_prio >= nrk_task_TCB[task_ID].task_prio)|| ) {
        //              NextNode = NextNode->Next;}
        // Stop if nextNode is freenode or next node less prio or (equal and elevated
        // Issues - 1 comes, becomes 2', 1 more comes (2' 1) then 2 comes where should it be placed ?
        // 2' 2  1 or 2 2' 1 in ready q , what happens after 2'->1, what if 2'->2

    }

    CurNode->task_ID = task_ID;
    _free_node = _free_node->Next;

    if (NextNode == _head_node)
    {
        //at start
        if (_head_node != NULL)
        {
            CurNode->Next = _head_node;
            CurNode->Prev = NULL;
            _head_node->Prev = CurNode;
        }
        else
        {
            CurNode->Next = NULL;
            CurNode->Prev = NULL;
            _free_node->Prev = CurNode;
        }
        _head_node = CurNode;

    }
    else
    {
        if (NextNode != _free_node)
        {
            // Insert  in middle

            CurNode->Prev = NextNode->Prev;
            CurNode->Next = NextNode;
            (NextNode->Prev)->Next = CurNode;
            NextNode->Prev = CurNode;
        }
        else
        {
            //insert at end
            CurNode->Next = NULL;
            CurNode->Prev = _free_node->Prev;
            _free_node->Prev = CurNode;
        }

    }

}


void nrk_rem_from_readyQ (int8_t task_ID)
{

    nrk_queue *CurNode;
//      nrk_queue       *tempNode;

//return;

    /*CurNode = _head_node;

       _head_node = _head_node->Next;
       _head_node->Prev = NULL;

       if (_free_node == NULL)
       {
       tempNode = _head_node;
       while (tempNode->Next!=NULL) tempNode=tempNode->Next;
       CurNode->Next = NULL;
       CurNode->Prev = tempNode;
       tempNode->Next = CurNode;
       _free_node = CurNode;
       }
       else
       {
       CurNode->Next = _free_node;
       _free_node->Prev = CurNode;
       _free_node = CurNode;
       }
     */

//      printf("nrk_rem_from_readyQ_nrk_queue %d\n",task_ID);

    if (_head_node == NULL)
        return;

    CurNode = _head_node;

    if (_head_node->task_ID == task_ID)
    {
        //REmove from start
        _head_node = _head_node->Next;
        _head_node->Prev = NULL;
    }
    else
    {
        while ((CurNode != NULL) && (CurNode->task_ID != task_ID))
            CurNode = CurNode->Next;
        if (CurNode == NULL)
            return;


        (CurNode->Prev)->Next = CurNode->Next;      //Both for middle and end
        if (CurNode->Next != NULL)
            (CurNode->Next)->Prev = CurNode->Prev;    // Only for middle

    }



    // Add to free list
    if (_free_node == NULL)
    {
        _free_node = CurNode;
        _free_node->Next = NULL;
    }
    else
    {
        CurNode->Next = _free_node;
        _free_node->Prev = CurNode;
        _free_node = CurNode;
    }
    _free_node->Prev = NULL;
}





nrk_status_t nrk_activate_task (nrk_task_type * Task)
{
    uint8_t rtype;
    NRK_STK *topOfStackPtr;

    topOfStackPtr = (NRK_STK *) nrk_task_stk_init (Task->task, Task->Ptos, Task->Pbos);

    //printf("activate %d\n",(int)Task.task_ID);
    if (Task->FirstActivation == TRUE)
    {
        rtype = nrk_TCB_init (Task, topOfStackPtr, Task->Pbos, 0, (void *) 0, 0);
        Task->FirstActivation = FALSE;

    }
    else
    {
        if (nrk_task_TCB[Task->task_ID].task_state != SUSPENDED)
            return NRK_ERROR;
        //Re-init some parts of TCB

        nrk_task_TCB[Task->task_ID].OSTaskStkPtr = (NRK_STK *) topOfStackPtr;



    }

    //nrk_task_TCB[Task->task_ID].task_state = READY;

    // Remove from suspended or waiting if extended

    // OSSchedLock();


    // If Idle Task then Add to ready Q
    //if(Task->task_ID==0) nrk_add_to_readyQ(Task->task_ID);
    //nrk_add_to_readyQ(Task->task_ID);
    //printf( "task %d nw %d \r\n",Task->task_ID,nrk_task_TCB[Task->task_ID].next_wakeup);
    //printf( "task %d nw %d \r\n",Task->task_ID,Task->offset.secs);
    if (nrk_task_TCB[Task->task_ID].next_wakeup == 0)
    {
        nrk_task_TCB[Task->task_ID].task_state = READY;
        nrk_add_to_readyQ (Task->task_ID);
    }

    return NRK_OK;
}



nrk_status_t nrk_terminate_task ()
{
    nrk_rem_from_readyQ (nrk_cur_task_TCB->task_ID);
    nrk_cur_task_TCB->task_state = FINISHED;

    // HAHA, there is NO next period...
    nrk_wait_until_next_period ();
    return NRK_OK;
}

int8_t nrk_wait_until_next_period ()
{
    uint8_t timer;
	 //Abhijeet disabled this...	Haww
		//NVIC_EnableIRQ(TIMER0_IRQn);
		//NVIC_EnableIRQ(TIMER1_IRQn);
    //nrk_stack_check ();
// Next Period Wakeup Time is Set inside scheduler when a task becomes Runnable
    nrk_int_disable ();
    nrk_cur_task_TCB->num_periods = 1;
    nrk_cur_task_TCB->suspend_flag = 1;
    timer = _nrk_os_timer_get ();

//nrk_cur_task_TCB->cpu_remaining=_nrk_prev_timer_val+1;

    if (timer < (MAX_SCHED_WAKEUP_TIME - TIME_PAD)) // if(timer<(250-10))
        if ((timer + TIME_PAD) <= _nrk_get_next_wakeup ())
        {
            timer += TIME_PAD;
            _nrk_prev_timer_val = timer;                  // pdiener: why is this only set in this special case?
            _nrk_set_next_wakeup (timer);                 // pdiener: Set next wakeup to NOW...Ask Madhur and Abhijeet
        }

    nrk_int_enable ();
    _nrk_wait_for_scheduler ();
		//pc6.printf("should not come here");
    return NRK_OK;
}

int8_t nrk_wait_until_next_n_periods (uint16_t p)
{
    uint8_t timer;

    nrk_stack_check ();

    if (p == 0)
        p = 1;
// Next Period Wakeup Time is Set inside scheduler when a task becomes Runnable
    nrk_int_disable ();
    nrk_cur_task_TCB->suspend_flag = 1;
    nrk_cur_task_TCB->num_periods = p;
    timer = _nrk_os_timer_get ();

//nrk_cur_task_TCB->cpu_remaining=_nrk_prev_timer_val+1;

// +2 allows for potential time conflict resolution
    if (timer < (MAX_SCHED_WAKEUP_TIME - TIME_PAD))       // 254 8bit overflow point - 2
        if ((timer + TIME_PAD) <= _nrk_get_next_wakeup ())
        {
            timer += TIME_PAD;
            _nrk_prev_timer_val = timer;
            _nrk_set_next_wakeup (timer);                 // pdiener: Set next wakeup to NOW
        }

    nrk_int_enable ();
    _nrk_wait_for_scheduler ();
    return NRK_OK;
}

/*
 * nrk_wait_ticks()
 *
 * This function will wait until a specified number of
 * timer ticks after the curret OS tick timer.
 *
 */

int8_t nrk_wait_ticks (uint16_t ticks)
{
    uint8_t timer;
    nrk_int_disable ();
    nrk_cur_task_TCB->suspend_flag = 1;
    timer = _nrk_os_timer_get ();
    nrk_cur_task_TCB->next_wakeup = ticks + timer;

    if (timer < MAX_SCHED_WAKEUP_TIME - TIME_PAD)
        if ((timer + TIME_PAD) <= _nrk_get_next_wakeup ())
        {
            timer += TIME_PAD;
            _nrk_prev_timer_val = timer;
            _nrk_set_next_wakeup (timer);
        }
//else nrk_cur_task_TCB->next_wakeup=ticks+1;
    nrk_int_enable ();
//while(nrk_cur_task_TCB->suspend_flag==1);
    _nrk_wait_for_scheduler ();
    return NRK_OK;
}


/*
 * nrk_wait_until_ticks()
 *
 * This function will wait until a specified number of
 * timer ticks starting from when the task was swapped in.
 * This means that this function can set periodic timing
 * taking into account any task processing time.
 *
 */

int8_t nrk_wait_until_ticks (uint16_t ticks)
{
    uint8_t timer;
    nrk_int_disable ();
    nrk_cur_task_TCB->suspend_flag = 1;
    nrk_cur_task_TCB->next_wakeup = ticks;
    // printf( "t %u\r\n",ticks );
    timer = _nrk_os_timer_get ();

    if (timer < MAX_SCHED_WAKEUP_TIME - TIME_PAD)
        if ((timer + TIME_PAD) <= _nrk_get_next_wakeup ())
        {
            timer += TIME_PAD;
            _nrk_prev_timer_val = timer;
            _nrk_set_next_wakeup (timer);
        }
//else nrk_cur_task_TCB->next_wakeup=ticks+1;
    nrk_int_enable ();
//while(nrk_cur_task_TCB->suspend_flag==1);
    _nrk_wait_for_scheduler ();
    return NRK_OK;
}

int8_t nrk_set_next_wakeup (nrk_time_t t)
{
    uint8_t timer;
    uint16_t nw;
    nrk_int_disable ();
    timer = _nrk_os_timer_get ();
    nw = _nrk_time_to_ticks (&t);
    if (nw <= TIME_PAD)
        return NRK_ERROR;
    nrk_cur_task_TCB->next_wakeup = nw + timer;
    /*    if(timer<(254-TIME_PAD))
            if((timer+TIME_PAD)<=_nrk_get_next_wakeup())
            {
                timer+=TIME_PAD;
                _nrk_prev_timer_val=timer;
                _nrk_set_next_wakeup(timer);
            }
    */
//      nrk_cur_task_TCB->nw_flag=1;
    nrk_int_enable ();

    return NRK_OK;
}

/*
 * nrk_wait_until_nw()
 *
 * This function will wait until a specified number of
 * timer ticks starting from when the task was swapped in.
 * This means that this function can set periodic timing
 * taking into account any task processing time.
 *
 */

int8_t nrk_wait_until_nw ()
{
    uint8_t timer;
    nrk_int_disable ();
    nrk_cur_task_TCB->suspend_flag = 1;
    nrk_cur_task_TCB->nw_flag = 1;
    timer = _nrk_os_timer_get ();

    if (timer < MAX_SCHED_WAKEUP_TIME - TIME_PAD)
        if ((timer + TIME_PAD) <= _nrk_get_next_wakeup ())
        {
            timer += TIME_PAD;
            _nrk_prev_timer_val = timer;
            _nrk_set_next_wakeup (timer);
        }
//else nrk_cur_task_TCB->next_wakeup=ticks+1;
    nrk_int_enable ();
//while(nrk_cur_task_TCB->suspend_flag==1);
    _nrk_wait_for_scheduler ();
    return NRK_OK;
}


int8_t nrk_wait (nrk_time_t t)
{
    uint8_t timer;
    uint16_t nw;

    nrk_stack_check ();

    nrk_int_disable ();
    nrk_cur_task_TCB->suspend_flag = 1;
    nrk_cur_task_TCB->num_periods = 1;
    timer = _nrk_os_timer_get ();

//printf( "t1 %lu %lu\n",t.secs, t.nano_secs/NANOS_PER_MS);

    nw = _nrk_time_to_ticks (&t);
// printf( "t2 %u %u\r\n",timer, nw);
    nrk_cur_task_TCB->next_wakeup = nw + timer;
//printf( "wu %u\n",nrk_cur_task_TCB->next_wakeup );
    if (timer < (MAX_SCHED_WAKEUP_TIME - TIME_PAD))
    {
        if ((timer + TIME_PAD) <= _nrk_get_next_wakeup ())
        {
            timer += TIME_PAD;
            _nrk_prev_timer_val = timer;
            _nrk_set_next_wakeup (timer);
        }
    }
    nrk_int_enable ();

    _nrk_wait_for_scheduler ();
    return NRK_OK;
}


inline void _nrk_wait_for_scheduler ()
{

    //TIMSK = BM (OCIE1A);
	
    do
    {
        //nrk_idle ();   
				//NVIC_SetPriority(PendSV_IRQn, 3);
				//NVIC_SetPriority(TIMER0_IRQn, 0);
				//NVIC_SetPriority(PendSV_IRQn, 1);
			  //wait_us(10);
				__WFI();// wait for the interrupt to tick... // pdiener: halt CPU here until any interrupt triggers
				//__WFE();
				//task_ID = nrk_get_high_ready_task_ID();
			/*	do
			 {
				//LPC_TIM0->IR |= (1 << 0); // Clear MR0 interrupt flag
			 }while(LPC_TIM0->IR & 0x01 == 0);
       */
				//sleep();
				
    }
    while ((volatile uint8_t) nrk_cur_task_TCB->suspend_flag == 1); // pdiener: make shure that was the right interrupt
		
    //TIMSK = BM (OCIE1A) | BM(OCIE0);
}


int8_t nrk_wait_until (nrk_time_t t)
{
    nrk_time_t ct;
    int8_t v;
//    uint8_t c;

    //c = _nrk_os_timer_get ();
    //do{
    //}while(_nrk_os_timer_get()==c);

    //ttt=c+1;
    nrk_time_get (&ct);

    v = nrk_time_sub (&t, t, ct);
    //nrk_time_compact_nanos(&t);
    if (v == NRK_ERROR)
    {
        return NRK_ERROR;
    }
//if(t.secs<ct.secs) return 0;
//if(t.secs==ct.secs && t.nano_secs<ct.nano_secs) return 0;

//t.secs-=ct.secs;
//t.nano_secs-=ct.nano_secs;
//
    nrk_wait (t);

    return NRK_OK;
}


uint8_t nrk_get_pid ()
{
    return nrk_cur_task_TCB->task_ID;
}
