#include <stm32f4xx_conf.h>
#include <hal.h>

void analog_config()
{
	/* GPIOC Peripheral clock enable. */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* Init GPIOB for ADC input */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void adc_config(void)
{
	/* Enable clock for ADC1 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	/* Init ADC1 */
	ADC_InitTypeDef ADC_InitStruct;
	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ExternalTrigConv = DISABLE;
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_NbrOfConversion = 1;
	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStruct.ADC_ScanConvMode = DISABLE;
	ADC_Init(ADC1, &ADC_InitStruct);
	ADC_Cmd(ADC1, ENABLE);
}

void adc_channel(uint8_t channel)
{
	/* select ADC channel */
	ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_84Cycles);
}

uint16_t adc_read()
{
	/* Start the conversion */
	ADC_SoftwareStartConv(ADC1);
	/* Wait until conversion completion */
	while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	/* Get the conversion value */
	return ADC_GetConversionValue(ADC1);
}
