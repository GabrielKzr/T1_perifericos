#include "adc_config.c"
#include "pwm.c"

#include <stm32f4xx_conf.h>
#include <hal.h>
#include <usart.h>
#include <ieee754.h>
#include <libc.h>

const float V_RAIL = 3300.0;		// 3300mV rail voltage
const float ADC_MAX = 4095.0;		// max ADC value
const int ADC_SAMPLES = 1024;		// ADC read samples
const int REF_RESISTANCE = 10000;	// 10k ohms

const float LUX_MIN = 0.3;
const float LUX_MAX = 325.0;

void set_pwm(float lux)
{
    int duty_cycle;

    if (lux < LUX_MIN) lux = LUX_MIN;
    if (lux > LUX_MAX) lux = LUX_MAX;

	printf("lux > %d\n", (int)lux);

    duty_cycle = (int)((1.0 - ((lux - LUX_MIN) / (LUX_MAX - LUX_MIN))) * 100.0);

	printf("dc > %d\n", duty_cycle);

    TIM4->CCR4 = duty_cycle;

	printf("PWM\n");
}

float luminosity()
{
	float voltage, lux = 0.0, rldr;
	
	for (int i = 0; i < ADC_SAMPLES; i++) {
		voltage = adc_read() * (V_RAIL / ADC_MAX);
		rldr = (REF_RESISTANCE * (V_RAIL - voltage)) / voltage;
		lux += 500 / (rldr / 650);
	}
	
	return (lux / ADC_SAMPLES);
}


int main(void)
{
	float _luminosity;
	char fval[30];
	
	uart_init(USART_PORT, 115200, 0);
	
	analog_config();
	adc_config();
	pwm_config();

	adc_channel(ADC_Channel_8);
	
	while (1) {
		_luminosity = luminosity();
		ftoa(_luminosity, fval, 6);
		printf("luminosity: %s\n", fval);
		set_pwm(_luminosity);
		
		delay_ms(1000);
	}
		
	return 0;
}

