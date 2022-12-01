/******************************************************************************
* File Name:   publisher_task.h
*
* Description: This file is the public interface of publisher_task.c
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

#ifndef PUBLISHER_TASK_H_
#define PUBLISHER_TASK_H_

/*******************************************************************************
* Macros
********************************************************************************/
#define PUBLISHER_TASK_PRIORITY   (2)
#define PUBLISHER_TASK_STACK_SIZE (1024 * 2)

#define MQTT_PUB_QUEUE_LENGTH (10u)
#define MQTT_PUB_MSG_MAX_SIZE (128u)
/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the Publisher Task. */
typedef enum
{
    PUBLISHER_INIT,
    PUBLISHER_DEINIT,
    PUBLISH_MQTT_MSG
} publisher_cmd_t;

/* Topics for the Publisher Task. */
typedef enum
{
    PRESENCE_STATUS,
    PRESENCE_EVENTS,
} presence_topic_t;

/* Struct to be passed via the publisher task queue */
typedef struct{
    publisher_cmd_t cmd;
    presence_topic_t topic;
    char data[MQTT_PUB_MSG_MAX_SIZE * 2];
} publisher_data_t;

/*******************************************************************************
* Extern Variables
********************************************************************************/


/*******************************************************************************
* Function Prototypes
********************************************************************************/
void publisher_task(void *pvParameters);

#endif /* PUBLISHER_TASK_H_ */

/* [] END OF FILE */
