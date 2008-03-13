/*
 * Copyright (C) 2006 Martin Willi
 * Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * $Id$
 */

/**
 * @defgroup task_manager task_manager
 * @{ @ingroup sa
 */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

typedef struct task_manager_t task_manager_t;

#include <library.h>
#include <encoding/message.h>
#include <sa/ike_sa.h>
#include <sa/tasks/task.h>

/**
 * First retransmit timeout in milliseconds.
 */
#define RETRANSMIT_TIMEOUT 4000

/**
 * Base which is raised to the power of the retransmission try.
 */
#define RETRANSMIT_BASE 1.8

/**
 * Number of retransmits done before giving up.
 */
#define RETRANSMIT_TRIES 5

/**
 * Interval for mobike routability checks in ms.
 */
#define ROUTEABILITY_CHECK_INTERVAL 2500

/**
 * Number of routability checks before giving up
 */
#define ROUTEABILITY_CHECK_TRIES 10


/**
 * The task manager, juggles task and handles message exchanges.
 *
 * On incoming requests, the task manager creates new tasks on demand and
 * juggles the request through all available tasks. Each task inspects the
 * request and adds payloads as necessary to the response.
 * On outgoing requests, the task manager delivers the request through the tasks
 * to build it, the response gets processed by each task to complete.
 * The task manager has an internal Queue to store task which should get
 * completed.
 * For the initial IKE_SA setup, several tasks are queued: One for the
 * unauthenticated IKE_SA setup, one for authentication, one for CHILD_SA setup
 * and maybe one for virtual IP assignement.
 * The task manager is also responsible for retransmission. It uses a backoff 
 * algorithm. The timeout is calculated using
 * RETRANSMIT_TIMEOUT * (RETRANSMIT_BASE ** try).
 * When try reaches RETRANSMIT_TRIES, retransmission is given up.
 *
 * Using an initial TIMEOUT of 4s, a BASE of 1.8, and 5 TRIES gives us:
 * @verbatim
                   | relative | absolute
   ---------------------------------------------------------
   4s * (1.8 ** 0) =    4s         4s
   4s * (1.8 ** 1) =    7s        11s
   4s * (1.8 ** 2) =   13s        24s
   4s * (1.8 ** 3) =   23s        47s
   4s * (1.8 ** 4) =   42s        89s
   4s * (1.8 ** 5) =   76s       165s
 
   @endberbatim
 * The peer is considered dead after 2min 45s when no reply comes in.
 */
struct task_manager_t {

	/**
	 * Process an incoming message.
	 * 
	 * @param message		message to add payloads to
	 * @return
	 * 						- DESTROY_ME if IKE_SA must be closed
	 *						- SUCCESS otherwise
	 */
	status_t (*process_message) (task_manager_t *this, message_t *message);

	/**
	 * Initiate an exchange with the currently queued tasks.
	 */
	status_t (*initiate) (task_manager_t *this);

	/**
	 * Queue a task in the manager.
	 *
	 * @param task			task to queue
	 */
	void (*queue_task) (task_manager_t *this, task_t *task);

	/**
	 * Retransmit a request if it hasn't been acknowledged yet.
	 *
	 * A return value of INVALID_STATE means that the message was already
	 * acknowledged and has not to be retransmitted. A return value of SUCCESS
	 * means retransmission was required and the message has been resent.
	 * 
	 * @param message_id	ID of the message to retransmit
	 * @return
	 * 						- INVALID_STATE if retransmission not required
	 *						- SUCCESS if retransmission sent
	 */
	status_t (*retransmit) (task_manager_t *this, u_int32_t message_id);

	/**
	 * Migrate all tasks from other to this.
	 *
	 * To rekey or reestablish an IKE_SA completely, all queued or active
	 * tasks should get migrated to the new IKE_SA.
	 * 
	 * @param other			manager which gives away its tasks
	 */
	void (*adopt_tasks) (task_manager_t *this, task_manager_t *other);
	
	/**
	 * Reset message ID counters of the task manager.
	 *
	 * The IKEv2 protocol requires to restart exchanges with message IDs
	 * reset to zero (INVALID_KE_PAYLOAD, COOKIES, ...). The reset() method
	 * resets the message IDs and resets all active tasks using the migrate()
	 * method.
	 * 
	 * @param other			manager which gives away its tasks
	 */
	void (*reset) (task_manager_t *this);
	
	/**
	 * Check if we are currently waiting for a reply.
	 *
	 * @return				TRUE if we are waiting, FALSE otherwise
	 */
	bool (*busy) (task_manager_t *this);
	
	/**
	 * Destroy the task_manager_t.
	 */
	void (*destroy) (task_manager_t *this);
};

/**
 * Create an instance of the task manager.
 *
 * @param ike_sa		IKE_SA to manage.
 */
task_manager_t *task_manager_create(ike_sa_t *ike_sa);

#endif /* TASK_MANAGER_H_ @} */
