/******************************************************************************
 * File Name:   radar_config_task.c
 *
 * Description: This file contains the task that handles parsing the new
 *              configuration coming from remote server and setting it to the
 *              xensiv-radar-presence library.
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

/* Header file from system */
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* Header file from library */
#include "cy_json_parser.h"

#include "rtos_artifacts.h"

/* Header file for local tasks */
#include "publisher_task.h"
#include "radar_config_task.h"
#include "radar_task.h"
#include "subscriber_task.h"

#include "xensiv_radar_presence.h"


/* Strings length */
#define MAX_INPUT_LENGTH  (50)
#define MAX_OUTPUT_LENGTH (100)

/* Strings for enable and disable */
#define ENABLE_STRING  ("enable")
#define DISABLE_STRING ("disable")

/* Max range min - max */
#define MAX_RANGE_MIN_LIMIT (0.66f)
#define MAX_RANGE_MAX_LIMIT (5.0f)

/* Macro threshold min - max */
#define MACRO_THRESHOLD_MIN_LIMIT (0.5f)
#define MACRO_THRESHOLD_MAX_LIMIT (2.0f)

/* Micro threshold min - max */
#define MICRO_THRESHOLD_MIN_LIMIT (0.2f)
#define MICRO_THRESHOLD_MAX_LIMIT (50.0f)

/* Names for presence parameters */
#define MAX_RANGE_STRING        ("max_range")
#define MACRO_THRESHOLD_STRING  ("macro_threshold")
#define MICRO_THRESHOLD_STRING  ("micro_threshold")
#define BANDPASS_STRING         ("bandpass_filter")
#define DECIMATION_STRING       ("decimation_filter")
#define MODE_STRING             ("mode")

/* Names for presence mode */
#define MACRO_ONLY_STRING      ("macro_only")
#define MICRO_ONLY_STRING      ("micro_only")
#define MICRO_IF_MACRO_STRING  ("micro_if_macro")
#define MICRO_AND_MACRO_STRING ("micro_and_macro")

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
TaskHandle_t radar_config_task_handle = NULL;

static publisher_data_t publisher_q_data =
{
    .cmd = PUBLISH_MQTT_MSG,
    .topic = PRESENCE_STATUS,
};

static publisher_data_t * publisher_msg = &publisher_q_data;

xensiv_radar_presence_config_t config;

float32_t binlength = 0.0f;
/*******************************************************************************
 * Function Name: check_bool_validation
 ********************************************************************************
 * Summary:
 *   Checks if entered value is a proper string for enable or disable
 *
 * Parameters:
 *   value : Entered string value
 *   enable : String value for enable
 *   disable : String value for disable
 *
 * Return:
 *   True if the value is same as enable or disable value, false if it is different
 *******************************************************************************/
static inline bool check_bool_validation(const char *value, const char *enable, const char *disable)
{
    bool result = false;

    if ((strcmp(value, enable) == 0) || (strcmp(value, disable) == 0))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: check_float_validation
 ********************************************************************************
 * Summary:
 *   Checks if entered value is within a range
 *
 * Parameters:
 *   value : Entered float value
 *   min : Minimum value
 *   max : Maximum value
 *
 * Return:
 *   True if the value is within a range, false if not
 *******************************************************************************/
static inline bool check_float_validation(float32_t value, float32_t min, float32_t max)
{
    bool result = false;

    if ((value >= min) && (value <= max))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: string_to_bool
 ********************************************************************************
 * Summary:
 *   Converts string value to true or false
 *
 * Parameters:
 *   string : Entered string value
 *   enable : String for enable value
 *   disable : String for disable value
 *
 * Return:
 *   True if entered string is enable, false if disable
 *******************************************************************************/
static inline bool string_to_bool(const char *string, const char *enable, const char *disable)
{
    assert((strcmp(string, enable) == 0) || (strcmp(string, disable) == 0));
    bool result = false;

    if (strcmp(string, enable) == 0)
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: check_mode_validation
 ********************************************************************************
 * Summary:
 *   Checks if entered value is a proper mode value
 *
 * Parameters:
 *   value : Entered mode value
 *
 * Return:
 *   True if the value is correct, false if the value is incorrect
 *******************************************************************************/
static inline bool check_mode_validation(const char *mode)
{
    bool result = false;

    if ((strcmp(mode, MACRO_ONLY_STRING) == 0) || (strcmp(mode, MICRO_ONLY_STRING) == 0) ||
        (strcmp(mode, MICRO_IF_MACRO_STRING) == 0) || (strcmp(mode, MICRO_AND_MACRO_STRING) == 0))
    {
        result = true;
    }
    else
    {
        result = false;
    }

    return result;
}

/*******************************************************************************
 * Function Name: string_to_mode
 ********************************************************************************
 * Summary:
 *   Translates string value into a numeral value for mode
 *
 * Parameters:
 *   value : Entered string
 *
 * Return:
 *   Presence mode value
 *******************************************************************************/
static inline xensiv_radar_presence_mode_t string_to_mode(const char *mode)
{
    xensiv_radar_presence_mode_t result = XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY;

    if (strcmp(mode, MACRO_ONLY_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY;
    }
    else if (strcmp(mode, MICRO_ONLY_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_ONLY;
    }
    else if (strcmp(mode, MICRO_IF_MACRO_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO;
    }
    else if (strcmp(mode, MICRO_AND_MACRO_STRING) == 0)
    {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_AND_MACRO;
    }
    else
    {
    }

    return result;
}


/*******************************************************************************
 * Function Name: json_parser_cb
 *******************************************************************************
 * Summary:
 *   Callback function that parses incoming json string.
 *
 * Parameters:
 *      json_object: incoming json object
 *      arg: callback data. error in parameter value
 *
 * Return:
 *   none
 ******************************************************************************/
static cy_rslt_t json_parser_cb(cy_JSON_object_t *json_object, void *arg)
{
    bool *config_error = (bool *)arg;
    json_object->value[json_object->value_length] = '\0';
    /* Supported keys and values for presence detection */
    if (memcmp(json_object->object_string, "max_range", json_object->object_string_length) == 0)
    {
        float32_t float_value = strtof(json_object->value, NULL);

        if (check_float_validation(float_value, MAX_RANGE_MIN_LIMIT, MAX_RANGE_MAX_LIMIT))
        {
            *config_error = false;
            config.max_range_bin =  (int32_t)(float_value/binlength);
            printf("new max_range value is: %f\r\n", float_value);
        }
        else
        {
            *config_error = true;
            printf("invalid max_range value\r\n");
        }
    }
    else if (memcmp(json_object->object_string, "macro_threshold", json_object->object_string_length) == 0)
    {
        float32_t float_value = strtof(json_object->value, NULL);

        if (check_float_validation(float_value, MACRO_THRESHOLD_MIN_LIMIT, MACRO_THRESHOLD_MAX_LIMIT))
        {
            *config_error = false;
            printf("new macro_threshold value is: %f\r\n", float_value);
            config.macro_threshold = float_value;
        }
        else
        {
            *config_error = true;
            printf("invalid macro_threshold value\r\n");
        }
    }
    else if (memcmp(json_object->object_string, "micro_threshold", json_object->object_string_length) == 0)
    {
        float32_t float_value = strtof(json_object->value, NULL);

        if (check_float_validation(float_value, MICRO_THRESHOLD_MIN_LIMIT, MICRO_THRESHOLD_MAX_LIMIT))
        {
            *config_error = false;
            printf("new micro_threshold value is: %f\r\n", float_value);
            config.micro_threshold = float_value;
        }
        else
        {
            *config_error = true;
            printf("invalid micro_threshold value\r\n");
        }
    }
    else if (memcmp(json_object->object_string, "bandpass_filter", json_object->object_string_length) == 0)
    {
        if (check_bool_validation(json_object->value, ENABLE_STRING, DISABLE_STRING))
        {
            *config_error = false;
            printf("new macro_fft_bandpass_filter value is: %s\r\n", json_object->value);
            config.macro_fft_bandpass_filter_enabled = string_to_bool(json_object->value, ENABLE_STRING, DISABLE_STRING);
            }
        else
        {
            *config_error = true;
            printf("invalid bandpass_filter value\r\n");
        }
    }
    else if (memcmp(json_object->object_string, "decimation_filter", json_object->object_string_length) == 0)
    {
        if (check_bool_validation(json_object->value, ENABLE_STRING, DISABLE_STRING))
        {
            *config_error = false;
            printf("new micro_fft_decimation value is: %s\r\n", json_object->value);
            config.micro_fft_decimation_enabled = string_to_bool(json_object->value, ENABLE_STRING, DISABLE_STRING);
        }
         else
         {
            *config_error = true;
            printf("invalid micro_fft_decimation_filter value\r\n");
         }
    }
    else if (memcmp(json_object->object_string, "mode", json_object->object_string_length) == 0)
    {
        if (check_mode_validation(json_object->value))
        {
            xensiv_radar_presence_mode_t mode = string_to_mode(json_object->value);
            *config_error = false;
            printf("new mode value is: %s\r\n", json_object->value);
            config.mode = mode;
        }
        else
        {
            *config_error = true;
            printf("invalid mode value\r\n");
        }
    }
    else
    {
        /* Invalid input json key */
        *config_error = true;
        printf("invalid parameter name\r\n");
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
 * Function Name: radar_config_task
 *******************************************************************************
 * Summary:
 *      Parse incoming json string, and set new configuration to
 *      presence library.
 *
 * Parameters:
 *   pvParameters: thread
 *
 * Return:
 *   none
 ******************************************************************************/
void radar_config_task(void *pvParameters)
{
    cy_rslt_t result;

    bool config_error = false;

    char *msg_payload;

    xensiv_radar_presence_handle_t handle = (xensiv_radar_presence_handle_t)pvParameters;

    /* Register JSON parser to parse input configuration JSON string */
    cy_JSON_parser_register_callback(json_parser_cb, (void *)&config_error);

    while (true)
    {
        /* Block till a notification is received from the subscriber task. */
        if (xQueueReceive(subscriber_msg_q, &msg_payload, portMAX_DELAY) == pdPASS )
        {
            /* Get mutex to block any other json parse jobs */
            if (xSemaphoreTake(sem_sub_payload, portMAX_DELAY) == pdTRUE)
            {
                
                result = xensiv_radar_presence_get_config(handle, &config);
                if (result != XENSIV_RADAR_PRESENCE_OK)
                {
                    printf("Error while reading presence config\r\n");
                }
                
                binlength = xensiv_radar_presence_get_bin_length(handle);

                result = cy_JSON_parser(msg_payload, strlen(msg_payload));
                if (result != CY_RSLT_SUCCESS)
                {
                    snprintf(publisher_q_data.data,
                             sizeof(publisher_q_data.data),
                             "{\"json parser error: invalid json message!\"}");
                             
                }
                else
                {
                    if(config_error)
                    {
                        snprintf(publisher_q_data.data,
                                 sizeof(publisher_q_data.data),
                                 "{\"error in configuration parameter name or invalid value/range!\"}");
                    }
                    else
                    {
                        /* Get mutex to block presence algorithm in radar task */
                        if (xSemaphoreTake(sem_radar_presence, portMAX_DELAY) == pdTRUE)
                        {
                            result = xensiv_radar_presence_set_config(handle, &config);
                            if (result != XENSIV_RADAR_PRESENCE_OK)
                            {
                                printf("Error while setting new presence config\r\n");
                                snprintf(publisher_q_data.data,
                                         sizeof(publisher_q_data.data),
                                         "{\"presence configuration could not be updated\"}");
                            }
                            else
                            {
                                snprintf(publisher_q_data.data,
                                         sizeof(publisher_q_data.data),
                                         "{\"presence configuration updated and application resumed\"}");
                                xensiv_radar_presence_reset(handle);
                            }

                            xSemaphoreGive(sem_radar_presence);
                        }
                        
                    }
                }                
                xSemaphoreGive(sem_sub_payload);
            }
            
            
            /* Send message back to publish queue. */
            xQueueSendToBack(publisher_task_q, &publisher_msg, 0);
        }
    }
}

/* [] END OF FILE */
