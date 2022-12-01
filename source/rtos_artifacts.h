/******************************************************************************
 * File Name:   radar_artifacts.h
 *
 * Description: This file contains the declaration of services used in the
 * application from FreeRTOS i.e. task, queues, and semaphores
 *
 * Related Document: See README.md
 *
 * ===========================================================================
 * Copyright (C) 2022 Infineon Technologies AG. All rights reserved.
 * ===========================================================================
 *
 * ===========================================================================
 * Infineon Technologies AG (INFINEON) is supplying this file for use
 * exclusively with Infineon's sensor products. This file can be freely
 * distributed within development tools and software supporting such
 * products.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON
 * WHATSOEVER.
 * ===========================================================================
 */

#ifndef SOURCE_RTOS_ARTIFACTS_H_
#define SOURCE_RTOS_ARTIFACTS_H_


/* Header file includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"


/*******************************************************************************
 * Macros
 ******************************************************************************/


/*******************************************************************************
 * Global Variables
 ******************************************************************************/
/* FreeRTOS task handle for radar task which performs radar data acquisition,
 *  runs presence algorithm, and puts presence events in publisher queue */
extern TaskHandle_t radar_task_handle;

/* FreeRTOS task handle for publisher task which receives messages in
 * publisher queue and transmits them to mqtt broker */
extern TaskHandle_t publisher_task_handle;

/* FreeRTOS task handle for subscriber task which subscribes to given topics
 * and receives data from remote broker*/
extern TaskHandle_t subscriber_task_handle;

/* FreeRTOS task handle for radar configuration task which receives messages
 * in subscriber queue, parse them. updates presence configuration, and send
 * status to publish queue */
extern TaskHandle_t radar_config_task_handle;

/* FreeRTOS queue handle for mqtt task queue used for mqtt related events */
extern QueueHandle_t mqtt_task_q;

/* FreeRTOS queue handle for publisher queue used to send/receive publish data */
extern QueueHandle_t publisher_task_q;

/* FreeRTOS queue handle for subscriber queue used to send/receive data to subscriber task */
extern QueueHandle_t subscriber_task_q;

/* FreeRTOS queue handle for subscriber_callback queue used for presence configuration */
extern QueueHandle_t subscriber_msg_q;

/* FreeRTOS semaphore handle to update/consume presence configuration data form subscriber topic */
extern SemaphoreHandle_t sem_sub_payload;

/* FreeRTOS semaphore handle to run/reset presence application */
extern SemaphoreHandle_t sem_radar_presence;

#endif /* SOURCE_RTOS_ARTIFACTS_H_ */
