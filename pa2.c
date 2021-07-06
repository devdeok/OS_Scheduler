/**********************************************************************
 * Copyright (c) 2019-2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

/* THIS FILE IS ALL YOURS; DO WHATEVER YOU WANT TO DO IN THIS FILE */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "types.h"
#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;


/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;


/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];


/**
 * Monotonically increasing ticks
 */
extern unsigned int ticks;


/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;


/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner) {
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_WAIT;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_WAIT);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

#include "sched.h"

/***********************************************************************
 * FIFO scheduler
 ***********************************************************************/
static int fifo_initialize(void)
{
	return 0;
}

static void fifo_finalize(void)
{
}

static struct process *fifo_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, when the current process is blocked
	 * while acquiring a resource, @current is (supposed to be) attached
	 * to the waitqueue of the corresponding resource. In this case just
	 * pick the next as well.
	 */
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan) { 
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue)) {
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note we use list_del_init()
		 * instead of list_del() to maintain the list head tidy. Otherwise,
		 * the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}	

	/* Return the next process to run */
	return next;
}

struct scheduler fifo_scheduler = {
	.name = "FIFO",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fifo_initialize,
	.finalize = fifo_finalize,
	.schedule = fifo_schedule,
};



/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void){
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;

	struct process *cur = NULL;
	struct process *curn = NULL;
	
	// 현재 process 상태가 wait이면 pick_next로 이동
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}
	
	// 현재 process를 계속 실행
	if (current->age < current->lifespan) {
		return current;
	}

	// 다음에 실행할 process를 분류함 -> 초기 lifespan이 제일 짧은 process
	pick_next:
	// readyqueue로 이동해서 비어있지 않다면 실행됨
	if (!list_empty(&readyqueue)) {
		// readyqueue에 process의 순번대로 next에 넣음
		next = list_first_entry(&readyqueue, struct process, list);	

		// 제일 작은 lifespan을 찾아서 next에 넣어주어야 함 
		list_for_each_entry_safe(cur,curn,&readyqueue,list){
			// readyqueue를 순회하여 제일 작은 lifespan을 next에 넣음
			if(next->lifespan > cur->lifespan)
				next = cur;
		}
		list_del_init(&next->list); //readyqueue에서 process 분리
	}	
	
	return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = sjf_schedule	/* TODO: Assign sjf_schedule()
								to this function pointer to activate
								SJF in the system */
};



/***********************************************************************
 * SRTF scheduler
 ***********************************************************************/
static struct process *srtf_schedule(void){
	/**
	 * Implement your own srtf scheduler here.
	 */
	struct process *next = NULL;
	// dump_status();

	struct process *cur = NULL;
	struct process *curn = NULL;

	// current process의 상태가 wait이면 pick_next로 이동
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}
	
	// 현재 process를 readyqueue 제일 끝에 붙임
	if (current->age < current->lifespan) {
			list_add_tail(&current->list,&readyqueue);
	}
	
	// 다음 실행할 process 분류 -> 남은 lifespan이 제일 짧은 process
	pick_next:
	if (!list_empty(&readyqueue)) {
		// readyqueue에 process의 순번대로 next에 넣음
		next = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry_safe(cur,curn,&readyqueue,list){
			// readyqueue를 순회하여 남은 lifespan 중 제일 작은 process를 next에 넣음
			if((next->lifespan-next->age) > (cur->lifespan-cur->age))
				next = cur;
		}

		list_del_init(&next->list); //readyqueue에서 process 분리
	}
	return next;
}

struct scheduler srtf_scheduler = {
	.name = "Shortest Remaining Time First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = srtf_schedule
	/* You need to check the newly created processes to implement SRTF.
	 * Use @forked() callback to mark newly created processes */
	/* Obviously, you should implement srtf_schedule() and attach it here */
};



/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process *rr_schedule(void){
	struct process *next = NULL;
	// dump_status();
	struct process *cur = NULL;
	struct process *curn = NULL;

	// current process의 상태가 wait이면 pick_next로 이동
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}
	
	// age가 lifespan보다 작다면
	if (current->age < current->lifespan) {
		// current가 한번 실행하고 다시 readyqueue에 붙임
		// 한번만 실행했으므로 time quantum인 1 tick 만족
		list_add_tail(&current->list,&readyqueue);
	}

	pick_next:
	if (!list_empty(&readyqueue)) {
		// fifo처럼 차례로 실행
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
	}
	return next;
}

// time quantum : 1 tick, 차례로 process를 실행
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
	.schedule = rr_schedule
	/* Obviously, you should implement rr_schedule() and attach it here */
};



/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
void prio_release(int resource_id){
	struct resource *r = resources + resource_id;
	struct process *cur, *curn;
	assert(r->owner == current);
	r->owner = NULL;

	// waitqueue에 있는 process들 중 제일 priority가 높은거 찾기
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);

		list_for_each_entry_safe(cur,curn,&r->waitqueue,list){
			if(waiter->prio < cur->prio){
				waiter = cur;
			}
		}

		assert(waiter->status == PROCESS_WAIT);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}

static struct process *prio_schedule(void){  
	struct process *next = NULL;
	dump_status();
	// acquire 1 0 2 -> 0번 했을 때 resource#1을 2 tick 사용
	struct process *cur = NULL;
	struct process *curn = NULL;

	// current process의 상태가 wait이면 pick_next로 이동
	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}
	
	if (current->age < current->lifespan) {
		list_add_tail(&current->list,&readyqueue);
	}
	
	// 다음에 실행할 process 분류 -> 우선순위가 높은 process
	pick_next:
	// readyqueu가 비어있지 않다면 실행
	if (!list_empty(&readyqueue)) {
		// readyqueue에 있는 process중에 priority가 제일 높은 process를 next에 넣어줌
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry_safe(cur,curn,&readyqueue,list){
			if(next->prio < cur->prio){
				next = cur;
			}
		}
		list_del_init(&next->list);
	}
	return next;
}

struct scheduler prio_scheduler = {
	.name = "Priority",
	.acquire = fcfs_acquire,
	.release = prio_release,
	.schedule = prio_schedule
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/

static struct process *pa_schedule(void){
	struct process *next = NULL;
	dump_status();
	struct process *cur = NULL;
	struct process *curn = NULL;

	if (!current || current->status == PROCESS_WAIT) {
		goto pick_next;
	}

	// 실행중인 process의 priority를 원래 priority로 되돌려 놓음
	if (current->age < current->lifespan) { 
		list_add_tail(&current->list,&readyqueue);
		current->prio = current->prio_orig;
	}

	pick_next:
		if (!list_empty(&readyqueue)) {
			// readyqueue에 있는 모든 process는 priority boost 1씩 받는다.
			// scheduling될 process는 priority가 높은 process
			next = list_first_entry(&readyqueue, struct process, list);
			list_for_each_entry_safe(cur,curn,&readyqueue,list){
				cur->prio++;

				if(next->prio < cur->prio){
					next = cur;
				}
			}
			list_del_init(&next->list);
		}	

	return next;
}


struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.schedule = pa_schedule
	/**
	 * Implement your own acqure/release function to make priority
	 * scheduler correct.
	 */
	/* Implement your own prio_schedule() and attach it here */
};



/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
bool pcp_acquire(int resource_id){
	struct resource *r = resources + resource_id;

	// resource의 owner가 없음
	if (!r->owner) {
		current->prio=MAX_PRIO;
		r->owner = current;
		return true;
	}
	current->status = PROCESS_WAIT;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

void pcp_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	assert(r->owner == current);
	current->prio = current->prio_orig;
	r->owner = NULL;

	if (!list_empty(&r->waitqueue)) {
		struct process *waiter =
				list_first_entry(&r->waitqueue, struct process, list);

		assert(waiter->status == PROCESS_WAIT);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}

struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	.acquire = pcp_acquire,
	.release = pcp_release,
	.schedule = prio_schedule
	/**
	 * Implement your own acqure/release function too to make priority
	 * scheduler correct.
	 */
};



/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
bool pip_acquire(int resource_id){
	struct resource *r = resources + resource_id;

	// resource의 owner가 없음
	if (!r->owner) {
		r->owner = current;
		return true;
	}
	r->owner->prio = current->prio;
	current->status = PROCESS_WAIT;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

void pip_release(int resource_id){
	struct resource *r = resources + resource_id;

	struct process *cur, *curn;

	assert(r->owner == current);
	r->owner->prio = current->prio_orig;
	r->owner = NULL;

	// waitqueue에 있는 process들 중 제일 priority가 높은거 찾기
	if (!list_empty(&r->waitqueue)) {
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);

		list_for_each_entry_safe(cur,curn,&r->waitqueue,list){
			if(waiter->prio < cur->prio){
				waiter = cur;
			}
		}

		assert(waiter->status == PROCESS_WAIT);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}


struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	.acquire = pip_acquire,
	.release = pip_release,
	.schedule = prio_schedule
	/**
	 * Ditto
	 */
};
