#include <stm32f4xx_conf.h>
#include <hal.h>
#include <usart.h>
#include <ieee754.h>
#include <libc.h>
#include <coos.h>
#include <malloc.h>


/* ADC library */
void analog_config();
void adc_config(void);
void adc_channel(uint8_t channel);
uint16_t adc_read();

/* PWM library */
void pwm_config();

/* sensors parameters */
const float F_VOLTAGE = 590.0;		// 590 ~ 700mV typical diode forward voltage
const float T_COEFF = -2.0;		// 1.8 ~ 2.2mV change per degree Celsius
const float V_RAIL = 3300.0;		// 3300mV rail voltage
const float ADC_MAX = 4095.0;		// max ADC value
const int ADC_SAMPLES = 1024;		// ADC read samples
const int REF_RESISTANCE = 10000;	// 4k7 ohms

const int LUX_MIN = 10;
const int LUX_MAX = 100;
const int DUTY_MAX = 1000;

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

float temperature()
{
	float temp = 0.0;
	float voltage;

	char fval[30];

	for (int i = 0; i < ADC_SAMPLES; i++) {
		voltage = adc_read() * (V_RAIL / ADC_MAX);
		temp += (((voltage - F_VOLTAGE) / T_COEFF));
	}

	ftoa(voltage, fval, 6);
	
	return (temp / ADC_SAMPLES);
}

void set_pwm_lux(float lux)
{
    int duty_cycle;

    if (lux < LUX_MIN) lux = LUX_MIN;
    if (lux > LUX_MAX) lux = LUX_MAX;

    duty_cycle = (1.0 - (lux - LUX_MIN) / (LUX_MAX - LUX_MIN)) * DUTY_MAX;

    TIM4->CCR4 = duty_cycle;
}

void set_pwm_temp(float temperature)
{
    int duty_cycle;

    if (temperature < 0) temperature = 0;
    if (temperature > 40) temperature = 40;

    duty_cycle = (temperature / 40.0) * DUTY_MAX;

    TIM4->CCR3 = duty_cycle;
}

/*
void set_pwm_temp(float temperature)
{
    int duty_cycle;
    static float smoothed_temp = 0;
	
    // Suavização
    smoothed_temp = 0.9 * smoothed_temp + 0.1 * temperature;
	
    if (smoothed_temp <= 20.0) {
        duty_cycle = 0;
    } else if (smoothed_temp >= 30.0) {
        duty_cycle = DUTY_MAX;
    } else {
        // Mapeia 20–30°C para 0–DUTY_MAX
        duty_cycle = ((smoothed_temp - 20.0) / 10.0) * DUTY_MAX;
    }
	
    TIM4->CCR3 = duty_cycle;
}
*/


/* application tasks */
void led_config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* GPIOC Peripheral clock enable. */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* configure board LED as output */
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}


void *t1(void *arg);
void *t2(void *arg);
void *t3(void *arg);
void *t4(void *arg);
void *t5(void *arg);

void *t1(void *arg)
{
	float* f = (float *)malloc(sizeof(float));
	
	static struct message_s msg;

	adc_channel(ADC_Channel_8);
	*f = temperature();	

	msg.type = 1;
	msg.data = f;
	task_mq_enqueue(t5, &msg);

	return 0;
}

void *t2(void *arg)
{
	float* f = (float *)malloc(sizeof(float));
	
	static struct message_s msg;
	
	adc_channel(ADC_Channel_9);
	*f = luminosity();	
	
	msg.type = 2;
	msg.data = f;

	task_mq_enqueue(t5, &msg);

	return 0;
}

void *t3(void *arg)
{	
	struct message_s *msg;
	float lux;

	if (task_mq_items() > 0)
	{
		msg = task_mq_dequeue();
			
		lux = *((float*)msg->data);
		set_pwm_lux(lux);

		free(msg->data);
	}

	return 0;
}

void *t4(void *arg)
{	
	struct message_s *msg;
	float temp;

	if (task_mq_items() > 0)
	{
		msg = task_mq_dequeue();
			
		temp = *((float*)msg->data);
		set_pwm_temp(temp);

		free(msg->data);
	}

	return 0;
}

void *t5(void *arg) {

	struct message_s *msg;
	char fval[30];

	while (task_mq_items() > 0)
	{
		msg = task_mq_dequeue();

		ftoa(*((float*)msg->data), fval, 6);

		if(msg->type == 1) {

			printf("temp > %s\n", fval);
			
			task_mq_enqueue(t4, msg);
			
		} else if(msg->type == 2) {

			printf("lux  > %s\n", fval);

			task_mq_enqueue(t3, msg);

		} else {
			printf("tipo não disponível\n");
		}

	}

	return 0;
}

int main(void)
{
	struct task_s tasks[MAX_TASKS] = { 0 };
	struct task_s *ptasks = tasks;

	/* configure board stuff */
	led_config();
	analog_config();
	adc_config();
	pwm_config();
	
	/* configure serial port */
	uart_init(USART_PORT, 115200, 0);

	/* setup CoOS and tasks */
	task_pinit(ptasks);
	task_add(ptasks, t1, 130);
	task_add(ptasks, t2, 130);

	// use of static memory in t1 and t2, so should be executed with less or equal priority

	task_add(ptasks, t3, 60);
	task_add(ptasks, t4, 60);
	task_add(ptasks, t5, 20);

	while (1) {
		task_schedule(ptasks);
		
		/* toggle board LED to show progress and wait for 1000ms */
		/* there is no need for LED toggle or delay in a real application */
		GPIO_ToggleBits(GPIOC, GPIO_Pin_13);
		// delay_ms(10);
	}
	
	/* never reached */
	return 0;
}

