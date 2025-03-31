#include <stm32f4xx_conf.h>
#include <hal.h>
#include <usart.h>
#include <ieee754.h>
#include <libc.h>
#include <coos.h>


/* ADC library */
void analog_config();
void adc_config(void);
void adc_channel(uint8_t channel);
uint16_t adc_read();

/* PWM library */
void pwm_config();

/* sensors parameters */
const float F_VOLTAGE = 635.0;		// 590 ~ 700mV typical diode forward voltage
const float T_COEFF = -2.0;		// 1.8 ~ 2.2mV change per degree Celsius
const float V_RAIL = 3300.0;		// 3300mV rail voltage
const float ADC_MAX = 4095.0;		// max ADC value
const int ADC_SAMPLES = 1024;		// ADC read samples
const int REF_RESISTANCE = 4700;	// 4k7 ohms

const float LUX_MIN = 0.3;
const float LUX_MAX = 325.0;

/* sensor aquisition functions */
float temperature()
{
	float temp = 0.0;
	float voltage;
	
	for (int i = 0; i < ADC_SAMPLES; i++) {
		voltage = adc_read() * (V_RAIL / ADC_MAX);
		temp += ((voltage - F_VOLTAGE) / T_COEFF);
	}
	
	return (temp / ADC_SAMPLES);
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

void *t1(void *arg)
{
	float f;
	char fval[30];
	
	adc_channel(ADC_Channel_8);
	f = temperature();	
	ftoa(f, fval, 6);
	printf("temp: %s\n", fval);

	return 0;
}

void *t2(void *arg)
{
	static float f;
	char fval[30];

	static struct message_s msg;
	struct message_s *pmsg;
	
	adc_channel(ADC_Channel_9);
	f = luminosity();	
	ftoa(f, fval, 6);
	printf("lux: %s\n", fval);

	pmsg = &msg;
	pmsg->data = &f;
	task_mq_enqueue(t3, pmsg);

	return 0;
}

void *t3(void *arg)
{	
	struct message_s *msg;
	float lux;

	printf("entering PWM \n");

	if (task_mq_items() > 0)
	{
		printf("setting pwm signal...\n");

		msg = task_mq_dequeue();

		if (msg) {
			
			lux = *((float*)msg->data);
			set_pwm(lux);

		} else {

			printf("Mensagem veio vazia\n");
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
	// task_add(ptasks, t1, 50);
	task_add(ptasks, t2, 50);
	task_add(ptasks, t3, 60);

	while (1) {
		task_schedule(ptasks);
		
		/* toggle board LED to show progress and wait for 1000ms */
		/* there is no need for LED toggle or delay in a real application */
		GPIO_ToggleBits(GPIOC, GPIO_Pin_13);
		delay_ms(1000);
	}
	
	/* never reached */
	return 0;
}

