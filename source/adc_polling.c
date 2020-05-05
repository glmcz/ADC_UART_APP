/*
 * Copyright (c) 2013 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_adc.h"
#include "fsl_lpuart.h"

#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_ADC_BASE ADC1
#define DEMO_ADC_USER_CHANNEL 0U
#define DEMO_ADC_CHANNEL_GROUP 0U
#define DAC_CONTROL_REG_ADD = 4009_4004
#define DMAE  0x02
#define ACFGT_ENABLE_LESS  0x08

//UART
#define DEMO_LPUART LPUART1
#define DEMO_LPUART_CLK_FREQ BOARD_DebugConsoleSrcFreq()
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
const uint32_t g_Adc_12bitFullRange = 4096U;
//uint8_t txbuff[4];
uint8_t txbuff[7]   = "m1204\r\n";
uint8_t endOfBuffer[2] = "\r\n";

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Main function
 */

static void delay(volatile uint32_t nof) {
  while(nof!=0) {
    __asm("NOP");
    nof--;
  }
}



int main(void)
{
    adc_config_t adcConfigStrcut;
    adc_channel_config_t adcChannelConfigStruct;
    adc_hardware_compare_config_t adc_COM_Conf;

    lpuart_config_t config;



    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    LPUART_GetDefaultConfig(&config);
//    config.parityMode = kLPUART_ParityEven;
    config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    LPUART_Init(DEMO_LPUART, &config, DEMO_LPUART_CLK_FREQ);


    PRINTF("\r\nADC polling Example.\r\n");


    /*
     *  config->enableAsynchronousClockOutput = true;
     *  config->enableOverWrite =               false;
     *  config->enableContinuousConversion =    false;
     *  config->enableHighSpeed =               false;
     *  config->enableLowPower =                false;
     *  config->enableLongSample =              false;
     *  config->referenceVoltageSource =        kADC_ReferenceVoltageSourceVref;
     *  config->samplePeriodMode =              kADC_SamplePeriod2or12Clocks;
     *  config->clockSource =                   kADC_ClockSourceAD;
     *  config->clockDriver =                   kADC_ClockDriver1;
     *  config->resolution =                    kADC_Resolution12Bit;
     */

    adcConfigStrcut.enableLongSample = true;
    adcConfigStrcut.samplePeriodMode = kADC_SamplePeriodLong12Clcoks;


	adc_COM_Conf.hardwareCompareMode =  kADC_HardwareCompareMode0; //  compare true if the result is less than the value1.
	adc_COM_Conf.value1 = 1700;



	ADC_GetDefaultConfig(&adcConfigStrcut);
	ADC_SetHardwareCompareConfig(DEMO_ADC_BASE,	&adc_COM_Conf ); // enable HW compare filter
    ADC_Init(DEMO_ADC_BASE, &adcConfigStrcut);

    ADC1->GC |= ACFGT_ENABLE_LESS;

#if !(defined(FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE) && FSL_FEATURE_ADC_SUPPORT_HARDWARE_TRIGGER_REMOVE)
    ADC_EnableHardwareTrigger(DEMO_ADC_BASE, false);
#endif

    // Do auto hardware calibration.
    if (kStatus_Success == ADC_DoAutoCalibration(DEMO_ADC_BASE))
    {
        PRINTF("ADC_DoAutoCalibration() Done.\r\n");
    }
    else
    {
        PRINTF("ADC_DoAutoCalibration() Failed.\r\n");
    }

     /* Configure the user channel and interrupt. */
    adcChannelConfigStruct.channelNumber                        = 0;
    adcChannelConfigStruct.enableInterruptOnConversionCompleted = false;
    GPIO_PinInit(GPIO1, 27, 0);


    PRINTF("ADC Full Range: %d\r\n", g_Adc_12bitFullRange);
    while (1)
    {
    	//PRINTF("Status flag %s \n",ADC_GetStatusFlags(DEMO_ADC_BASE));
    	delay(100000);
        //PRINTF("Press any key to get user channel's ADC value.\r\n");
        delay(100000);
        /*
         When in software trigger mode, each conversion would be launched once calling the "ADC_ChannelConfigure()"
         function, which works like writing a conversion command and executing it. For another channel's conversion,
         just to change the "channelNumber" field in channel's configuration structure, and call the
         "ADC_ChannelConfigure() again.
        */
        ADC_DoAutoCalibration(DEMO_ADC_BASE);
        ADC_SetChannelConfig(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP, &adcChannelConfigStruct);
        while (0U == ADC_GetChannelStatusFlags(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP))
        {
        }
        uint8_t voltage1 = ADC_GetChannelConversionValue(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP);
        uint32_t voltage = ADC_GetChannelConversionValue(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP);
        PRINTF("%d\r\n", voltage);
              /* pack into buf string */
              txbuff[0] = voltage >> 24;
              txbuff[1] = voltage >> 16;
              txbuff[2] = voltage >> 8;
              txbuff[3] = voltage;

        for(int i = 3; i >= 0; i--){
             txbuff[i] = voltage % 10;  // remainder of division with 10 gets the last digit
             voltage /= 10;     // dividing by ten chops off the last digit.
        }
        // Implementation of the non-standard 'itoa' and 'uitoa' functions,
        // which support bases between 2 and 16 (2=binary, 10=decimal,
        // 16=hexadecimal).
        //sprintf(txbuff, "%d", voltage);
        //int len = sizeof(txbuff) + sizeof(endOfBuffer);
        //uint8_t join[len];
        //sprintf(join, "%s%c", txbuff, endOfBuffer);
        itoa(voltage, txbuff, 10);
        LPUART_WriteBlocking(DEMO_LPUART, txbuff,  sizeof(txbuff));
//        PRINTF("ADC Value: %d\r\n", ADC_GetChannelConversionValue(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP));
    }
}
