/******************************************************************************
* File Name:   publisher_task.c
*
* Description: This file contains the task that sets up the user button GPIO 
*              for the publisher and publishes MQTT messages on the topic
*              'MQTT_PUB_TOPIC' to control a device that is actuated by the
*              subscriber task. The file also contains the ISR that notifies
*              the publisher task about the new device state to be published.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2022, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"

#include "rtos_artifacts.h"

/* Task header files */
#include "publisher_task.h"
#include "mqtt_task.h"
#include "subscriber_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"

/******************************************************************************
* Macros
******************************************************************************/

/* The maximum number of times each PUBLISH in this example will be retried. */
#define PUBLISH_RETRY_LIMIT             (10)

/* A PUBLISH message is retried if no response is received within this 
 * time (in milliseconds).
 */
#define PUBLISH_RETRY_MS                (1000)

/* Queue length of a message queue that is used to communicate with the 
 * publisher task.
 */
#define PUBLISHER_TASK_QUEUE_LENGTH     (3u)

/******************************************************************************
* Function Prototypes
*******************************************************************************/


/******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Handle of the queue holding the commands for the publisher task */
QueueHandle_t publisher_task_q;

/* Structure to store publish message information. */
cy_mqtt_publish_info_t publish_info[2] =
{
    {.qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_PUB_TOPIC_STATUS,
    .topic_len = (sizeof(MQTT_PUB_TOPIC_STATUS) - 1),
    .retain = false,
    .dup = false},
    {.qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
    .topic = MQTT_PUB_TOPIC_EVENTS,
    .topic_len = (sizeof(MQTT_PUB_TOPIC_EVENTS) - 1),
    .retain = false,
    .dup = false},
};


/******************************************************************************
 * Function Name: publisher_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the user button GPIO for the publisher and publishes 
 *  MQTT messages to the broker. The user button init and deinit operations,
 *  and the MQTT publish operation is performed based on commands sent by other
 *  tasks and callbacks over a message queue.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void publisher_task(void *pvParameters)
{
    /* Status variable */
    cy_rslt_t result;

    cy_mqtt_t mqtt_connection = (cy_mqtt_t ) pvParameters;

    publisher_data_t *publisher_q_data;

    /* Command to the MQTT client task */
    mqtt_task_cmd_t mqtt_task_cmd;

    /* Create a message queue to communicate with other tasks and callbacks. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof( struct publisher_data_t * ));

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data->cmd)
            {
                case PUBLISHER_INIT:
                {
                    /* Reserved for customer extension. */
                    printf("  Publisher: init event occurred\n\n");
                    break;
                }

                case PUBLISHER_DEINIT:
                {
                    printf("  Publisher: deinit event occurred\n\n");
                    /* Reserved for customer extension. */
                    break;
                }

                case PUBLISH_MQTT_MSG:
                {
                    uint8_t topic_idx = publisher_q_data->topic;

                    /* Publish the data received over the message queue. */
                    publish_info[topic_idx].payload = publisher_q_data->data;
                    publish_info[topic_idx].payload_len = strlen(publish_info[topic_idx].payload);

                    printf("  Publisher: Publishing '%s' on the topic '%s'\n\n",
                           (char *) publish_info[topic_idx].payload, publish_info[topic_idx].topic);

                    result = cy_mqtt_publish(mqtt_connection, &publish_info[topic_idx]);

                    if (result != CY_RSLT_SUCCESS)
                    {
                        printf("  Publisher: MQTT Publish failed with error 0x%0X.\n\n", (int)result);

                        /* Communicate the publish failure with the the MQTT 
                         * client task.
                         */
                        mqtt_task_cmd = HANDLE_MQTT_PUBLISH_FAILURE;
                        xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
                    }

                    break;
                }
            }
        }
    }
}




/* [] END OF FILE */
