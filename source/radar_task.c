/*****************************************************************************
 * File name: radar_task.c
 *
 * Description: This file uses RadarSensing library APIs to demonstrate
 * presence detection use case of radar.
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

/* Header file includes */
#include "cybsp.h"
#include "cyhal.h"

#include "rtos_artifacts.h"

/* Header file for local task */
#include "publisher_task.h"
#include "radar_config_task.h"

#include "radar_task.h"

#include "xensiv_bgt60trxx_mtb.h"
#include "xensiv_radar_presence.h"

#define XENSIV_BGT60TRXX_CONF_IMPL
#include "radar_settings.h"
/*******************************************************************************
 * Macros
 ******************************************************************************/

#define LED_RGB_RED                         CYBSP_GPIOA0
#define LED_RGB_GREEN                       CYBSP_GPIOA1
#define LED_RGB_BLUE                        CYBSP_GPIOA2

#define PIN_XENSIV_BGT60TRXX_SPI_SCLK       CYBSP_SPI_CLK
#define PIN_XENSIV_BGT60TRXX_SPI_MOSI       CYBSP_SPI_MOSI
#define PIN_XENSIV_BGT60TRXX_SPI_MISO       CYBSP_SPI_MISO
#define PIN_XENSIV_BGT60TRXX_SPI_CSN        CYBSP_SPI_CS
#define PIN_XENSIV_BGT60TRXX_IRQ            CYBSP_GPIO10
#define PIN_XENSIV_BGT60TRXX_RSTN           CYBSP_GPIO11
#define PIN_XENSIV_BGT60TRXX_LDO_EN         CYBSP_GPIO5

#define XENSIV_BGT60TRXX_SPI_FREQUENCY      (25000000UL)
#define XENSIV_BGT60TRXX_LDO_DELAY_MS       (5)

#define NUM_SAMPLES_PER_FRAME               (XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)

#define NUM_CHIRPS_PER_FRAME                XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME
#define NUM_SAMPLES_PER_CHIRP               XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP

#define GPIO_INTERRUPT_PRIORITY             (6)



/*******************************************************************************
 * Global Variables
 ******************************************************************************/
TaskHandle_t radar_task_handle = NULL;

/* Semaphore to protect radar sensing context */
SemaphoreHandle_t sem_radar_presence = NULL;

static cyhal_spi_t spi_obj;
static xensiv_bgt60trxx_mtb_t bgt60_obj;
static uint16_t bgt60_buffer[NUM_SAMPLES_PER_FRAME] __attribute__((aligned(2)));
static float32_t frame[NUM_SAMPLES_PER_FRAME];
static float32_t avg_chirp[NUM_SAMPLES_PER_CHIRP];

static publisher_data_t publisher_q_data;
static publisher_data_t * publisher_msg = &publisher_q_data;
/*******************************************************************************
 * Local Variables
 ******************************************************************************/

/*******************************************************************************
* Function Name: xensiv_bgt60trxx_interrupt_handler
********************************************************************************
* Summary:
* This is the interrupt handler to react on sensor indicating the availability
* of new data
*    1. Notifies main task on interrupt from sensor
*
* Parameters:
*  void
*
* Return:
*  none
*
*******************************************************************************/
#if defined(CYHAL_API_VERSION) && (CYHAL_API_VERSION >= 2)
static void xensiv_bgt60trxx_interrupt_handler(void *args, cyhal_gpio_event_t event)
#else
static void xensiv_bgt60trxx_interrupt_handler(void *args, cyhal_gpio_irq_event_t event)
#endif
{
    CY_UNUSED_PARAMETER(args);
    CY_UNUSED_PARAMETER(event);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(radar_task_handle, &xHigherPriorityTaskWoken);

    /* Context switch needed? */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
/*******************************************************************************
* Function Name: presence_detection_cb
********************************************************************************
* Summary:
* This is the callback function o indicate presence/absence events on terminal/cloud
* and LEDs.
* Parameters:
*  void
*
* Return:
*  None
*
*******************************************************************************/
void presence_detection_cb(xensiv_radar_presence_handle_t handle,
                           const xensiv_radar_presence_event_t* event,
                           void *data)
{
    (void)data;
    (void)handle;

    publisher_q_data.cmd = PUBLISH_MQTT_MSG;
    publisher_q_data.topic =  PRESENCE_EVENTS;

    switch (event->state)
    {
        case XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE:
            cyhal_gpio_write(LED_RGB_RED, true);
            cyhal_gpio_write(LED_RGB_GREEN, false);
            printf("[INFO] macro presence %" PRIi32 " %" PRIi32 "\n",
                   event->range_bin,
                   event->timestamp);

            snprintf(publisher_q_data.data, sizeof(publisher_q_data.data), "{\"PRESENCE\": \"IN macro\"}");
            break;

        case XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE:
            cyhal_gpio_write(LED_RGB_RED, true);
            cyhal_gpio_write(LED_RGB_GREEN, false);
            printf("[INFO] micro presence %" PRIi32 " %" PRIi32 "\n",
                   event->range_bin,
                   event->timestamp);

            snprintf(publisher_q_data.data, sizeof(publisher_q_data.data), "{\"PRESENCE\": \"IN micro\"}");
            break;

        case XENSIV_RADAR_PRESENCE_STATE_ABSENCE:
            printf("[INFO] absence %" PRIu32 "\n", event->timestamp);
            cyhal_gpio_write(LED_RGB_RED, false);
            cyhal_gpio_write(LED_RGB_GREEN, true);

            snprintf(publisher_q_data.data, sizeof(publisher_q_data.data), "{\"PRESENCE\": \"OUT\"}");
            break;

        default:
            printf("[WARN]: Unknown reported state in event handling\n");
            break;
    }

    /* Send message back to publish queue. */
    xQueueSendToBack(publisher_task_q, &publisher_msg, 0 );
}


/*******************************************************************************
* Function Name: init_leds
********************************************************************************
* Summary:
* This function initializes the GPIOs for LEDs and set them to off state.
* Parameters:
*  void
*
* Return:
*  Success or error
*
*******************************************************************************/
static int32_t init_leds(void)
{

    if(cyhal_gpio_init(LED_RGB_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("ERROR: GPIO initialization for LED_RGB_RED failed\n");
        return -1;
    }

    if( cyhal_gpio_init(LED_RGB_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("ERROR: GPIO initialization for LED_RGB_GREEN failed\n");
        return -1;
    }

    if( cyhal_gpio_init(LED_RGB_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false)!= CY_RSLT_SUCCESS)
    {
        printf("ERROR: GPIO initialization for LED_RGB_BLUE failed\n");
        return -1;
    }

    return 0;
}

/*******************************************************************************
* Function Name: init_sensor
********************************************************************************
* Summary:
* This function configures the SPI interface, initializes radar and interrupt
* service routine to indicate the availability of radar data.
*
* Parameters:
*  void
*
* Return:
*  Success or error
*
*******************************************************************************/
static int32_t init_sensor(void)
{
    if (cyhal_spi_init(&spi_obj,
                       PIN_XENSIV_BGT60TRXX_SPI_MOSI,
                       PIN_XENSIV_BGT60TRXX_SPI_MISO,
                       PIN_XENSIV_BGT60TRXX_SPI_SCLK,
                       NC,
                       NULL,
                       8,
                       CYHAL_SPI_MODE_00_MSB,
                       false) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: cyhal_spi_init failed\n");
        return -1;
    }

    /* Reduce drive strength to improve EMI */
    Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_DRIVE_1_8);
    Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_DRIVE_1_8);

    /* Set the data rate to 25 Mbps */
    if (cyhal_spi_set_frequency(&spi_obj, XENSIV_BGT60TRXX_SPI_FREQUENCY) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: cyhal_spi_set_frequency failed\n");
        return -1;
    }

    /* Enable LDO */
    if (cyhal_gpio_init(PIN_XENSIV_BGT60TRXX_LDO_EN,
                        CYHAL_GPIO_DIR_OUTPUT,
                        CYHAL_GPIO_DRIVE_STRONG,
                        true) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: LDO_EN cyhal_gpio_init failed\n");
        return -1;
    }

    /* Wait LDO stable */
    (void)cyhal_system_delay_ms(XENSIV_BGT60TRXX_LDO_DELAY_MS);

    if (xensiv_bgt60trxx_mtb_init(&bgt60_obj,
                                  &spi_obj,
                                  PIN_XENSIV_BGT60TRXX_SPI_CSN,
                                  PIN_XENSIV_BGT60TRXX_RSTN,
                                  register_list,
                                  XENSIV_BGT60TRXX_CONF_NUM_REGS) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: xensiv_bgt60trxx_mtb_init failed\n");
        return -1;
    }

    if (xensiv_bgt60trxx_mtb_interrupt_init(&bgt60_obj,
                                            NUM_SAMPLES_PER_FRAME,
                                            PIN_XENSIV_BGT60TRXX_IRQ,
                                            GPIO_INTERRUPT_PRIORITY,
                                            xensiv_bgt60trxx_interrupt_handler,
                                            NULL) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: xensiv_bgt60trxx_mtb_interrupt_init failed\n");
        return -1;
    }

    return 0;
}

/*******************************************************************************
 * Function Name: radar_task
 *******************************************************************************
 * Summary:
 *   Initializes GPIO ports, context object of presence
 *   detection application, then initializes radar device configuration,
 *   sets parameters for presence detection, registers
 *   callback to handle presence detection events and
 *   continuously processes data acquired from radar.
 *
 * Parameters:
 *   pvParameters: thread
 *
 * Return:
 *   none
 ******************************************************************************/
void radar_task(void *pvParameters)
{

    (void)pvParameters;

    xensiv_radar_presence_handle_t handle;

    static const xensiv_radar_presence_config_t default_config =
    {
        .bandwidth                         = 460E6,
        .num_samples_per_chirp             = XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP,
        .micro_fft_decimation_enabled      = false,
        .micro_fft_size                    = 128,
        .macro_threshold                   = 0.5f,
        .micro_threshold                   = 12.5f,
        .min_range_bin                     = 1,
        .max_range_bin                     = 5,
        .macro_compare_interval_ms         = 250,
        .macro_movement_validity_ms        = 1000,
        .micro_movement_validity_ms        = 4000,
        .macro_movement_confirmations      = 0,
        .macro_trigger_range               = 1,
        .mode                              = XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO,
        .macro_fft_bandpass_filter_enabled = false,
        .micro_movement_compare_idx       = 5
    };


    if (init_sensor() != 0)
    {
        CY_ASSERT(0);
    }

    if (init_leds () != 0)
    {
        CY_ASSERT(0);
    }

    xensiv_radar_presence_set_malloc_free(pvPortMalloc,
                                          vPortFree);

    if (xensiv_radar_presence_alloc(&handle, &default_config) != 0)
    {
        CY_ASSERT(0);
    }

    xensiv_radar_presence_set_callback(handle, presence_detection_cb, NULL);

    /* Initiate semaphore mutex to protect 'radar_presence_context' */
    sem_radar_presence = xSemaphoreCreateMutex();
    if (sem_radar_presence == NULL)
    {
        printf(" 'sem_radar_presence' semaphore creation failed... Task suspend\n\n");
        vTaskSuspend(NULL);
    }


    /**
     * Create task for radar configuration. Configuration parameters come from
     * Subscriber task. 
     */
    if (pdPASS != xTaskCreate(radar_config_task,
                              RADAR_CONFIG_TASK_NAME,
                              RADAR_CONFIG_TASK_STACK_SIZE,
                              handle,
                              RADAR_CONFIG_TASK_PRIORITY,
                              &radar_config_task_handle))
    {
        printf("Failed to create Radar config task!\n");
        CY_ASSERT(0);
    }

    if (xensiv_bgt60trxx_start_frame(&bgt60_obj.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        CY_ASSERT(0);
    }

    printf("Presence application running \n\n");

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (xensiv_bgt60trxx_get_fifo_data(&bgt60_obj.dev,
                                            bgt60_buffer,
                                            NUM_SAMPLES_PER_FRAME) == XENSIV_BGT60TRXX_STATUS_OK)
                                            
        {
            /* Data preprocessing */
            uint16_t *bgt60_buffer_ptr = &bgt60_buffer[0];
            float32_t *frame_ptr = &frame[0];
            for (int32_t sample = 0; sample < NUM_SAMPLES_PER_FRAME; ++sample)
            {
                *frame_ptr++ = ((float32_t)(*bgt60_buffer_ptr++) / 4096.0F);
            }

            // calculate the average of the chirps first
            arm_fill_f32(0, avg_chirp, NUM_SAMPLES_PER_CHIRP);

            for (int chirp = 0; chirp < NUM_CHIRPS_PER_FRAME; chirp++)
            {
                arm_add_f32(avg_chirp, &frame[NUM_SAMPLES_PER_CHIRP * chirp], avg_chirp, NUM_SAMPLES_PER_CHIRP);
            }

            arm_scale_f32(avg_chirp, 1.0f / NUM_CHIRPS_PER_FRAME, avg_chirp, NUM_SAMPLES_PER_CHIRP);
                
            if (xSemaphoreTake(sem_radar_presence, portMAX_DELAY) == pdTRUE)
            {
                if((xensiv_radar_presence_process_frame(handle, frame, xTaskGetTickCount() * portTICK_PERIOD_MS)) != XENSIV_RADAR_PRESENCE_OK)
                {
                    printf("Failed during frame processing\n");
                }
                
                xSemaphoreGive(sem_radar_presence);
            }
            
            
        }
    }
}

/*******************************************************************************
 * Function Name: radar_task_cleanup
 *******************************************************************************
 * Summary:
 *   Cleanup all resources radar_task has used/created.
 *
 * Parameters:
 *   void
 *
 * Return:
 *   void
 ******************************************************************************/
void radar_task_cleanup(void)
{
    if (radar_config_task_handle != NULL)
    {
        vTaskDelete(radar_config_task_handle);
    }
}

/* [] END OF FILE */
