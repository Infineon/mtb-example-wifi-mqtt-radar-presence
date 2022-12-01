/******************************************************************************
 * File Name:   radar_task.h
 *
 * Description: This file contains the function prototypes and constants used
 *   in radar_task.c.
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

#ifndef RADAR_TASK_H_
#define RADAR_TASK_H_

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define RADAR_TASK_NAME       "RADAR PRESENCE TASK"
#define RADAR_TASK_STACK_SIZE (1024 * 4)
#define RADAR_TASK_PRIORITY   (3)

/*******************************************************************************
 * Global Variables
 ******************************************************************************/


/*******************************************************************************
 * Functions
 ******************************************************************************/
void radar_task(void *pvParameters);
void radar_task_cleanup(void);

#endif
/* [] END OF FILE */
