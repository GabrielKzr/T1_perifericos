CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
DUMP = arm-none-eabi-objdump
READ = arm-none-eabi-readelf
OBJ = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
AR = arm-none-eabi-ar

#APPLICATION	= app
APPLICATION	= coos_stackless
#APPLICATION	= coos_stackful
PROG		= firmware

PROJECT_DIR	= .
HAL_DIR		= $(PROJECT_DIR)/src/hal
CMSIS_DIR	= $(PROJECT_DIR)/src/cmsis
USBCDC_DIR	= $(PROJECT_DIR)/src/usb_cdc
COOS_STACKLESS_DIR	= $(PROJECT_DIR)/src/coos/stackless
COOS_STACKFUL_DIR	= $(PROJECT_DIR)/src/coos/stackful

# this is stuff specific to this architecture
INC_DIRS  = \
	-I $(HAL_DIR) \
	-I $(CMSIS_DIR)/core \
	-I $(CMSIS_DIR)/device \
	-I $(USBCDC_DIR)

# serial port
SERIAL_DEV = /dev/ttyACM0
# uart baud rate
SERIAL_BR = 115200
# uart port
SERIAL_PORT = 0

#remove unreferenced functions
CFLAGS_STRIP = -fdata-sections -ffunction-sections
LDFLAGS_STRIP = --gc-sections

# this is stuff used everywhere - compiler and flags should be declared (ASFLAGS, CFLAGS, LDFLAGS, LD_SCRIPT, CC, AS, LD, DUMP, READ, OBJ and SIZE).
MCU_DEFINES = -mcpu=cortex-m4 -mtune=cortex-m4 -mfloat-abi=hard -mthumb -fsingle-precision-constant -mfpu=fpv4-sp-d16 -Wdouble-promotion
#MCU_DEFINES = -mcpu=cortex-m4 -mtune=cortex-m4 -mfloat-abi=soft -mabi=atpcs -mthumb -fsingle-precision-constant
C_DEFINES = -D STM32F401xC -D HSE_VALUE=25000000 -D LITTLE_ENDIAN
#C_DEFINES = -D STM32F411xE -D HSE_VALUE=25000000 -D LITTLE_ENDIAN
#C_DEFINES = -D STM32F407xx -D HSE_VALUE=8000000 -D LITTLE_ENDIAN
CFLAGS = -Wall -O2 -c $(MCU_DEFINES) -mapcs-frame -fverbose-asm -nostdlib -ffreestanding $(C_DEFINES) $(INC_DIRS) -D USART_BAUD=$(SERIAL_BR) -D USART_PORT=$(SERIAL_PORT) $(CFLAGS_STRIP)

LDFLAGS = $(LDFLAGS_STRIP)
LDSCRIPT = $(HAL_DIR)/stm32f401_flash.ld

CMSIS_SRC = \
	$(CMSIS_DIR)/device/stm32f4xx_rcc.c \
	$(CMSIS_DIR)/device/stm32f4xx_gpio.c \
	$(CMSIS_DIR)/device/stm32f4xx_tim.c \
	$(CMSIS_DIR)/device/stm32f4xx_adc.c \
	$(CMSIS_DIR)/device/stm32f4xx_i2c.c \
	$(CMSIS_DIR)/device/stm32f4xx_spi.c \
	$(CMSIS_DIR)/device/stm32f4xx_usart.c \
	$(CMSIS_DIR)/device/stm32f4xx_syscfg.c \
	$(CMSIS_DIR)/device/stm32f4xx_exti.c \
	$(CMSIS_DIR)/device/misc.c \
	$(CMSIS_DIR)/device/system_stm32f4xx.c

USBCDC_SRC = \
	$(USBCDC_DIR)/usb_bsp.c \
	$(USBCDC_DIR)/usb_core.c \
	$(USBCDC_DIR)/usb_dcd.c \
	$(USBCDC_DIR)/usb_dcd_int.c \
	$(USBCDC_DIR)/usbd_cdc_core.c \
	$(USBCDC_DIR)/usbd_cdc_vcp.c \
	$(USBCDC_DIR)/usbd_core.c \
	$(USBCDC_DIR)/usbd_desc.c \
	$(USBCDC_DIR)/usbd_ioreq.c \
	$(USBCDC_DIR)/usbd_req.c \
	$(USBCDC_DIR)/usbd_usr.c

HAL_SRC = \
	$(HAL_DIR)/setjmp.s \
	$(HAL_DIR)/aeabi.s \
	$(HAL_DIR)/muldiv.c \
	$(HAL_DIR)/ieee754.c \
	$(HAL_DIR)/stm32f4_vector.c \
	$(HAL_DIR)/usart.c \
	$(HAL_DIR)/libc.c \
	$(HAL_DIR)/malloc.c

COOS_STACKLESS_SRC = \
	$(COOS_STACKLESS_DIR)/coos.c
	
COOS_STACKFUL_SRC = \
	$(COOS_STACKFUL_DIR)/coos.c

APP_SRC = \
	$(PROJECT_DIR)/src/main_coos_stackful.c
	
APP_SRC_STACKLESS = \
	$(PROJECT_DIR)/src/main.c \
	$(PROJECT_DIR)/src/pwm.c \
	$(PROJECT_DIR)/src/adc.c  	
	
APP_SRC_STACKFUL = \
	$(PROJECT_DIR)/src/main_coos_stackful.c

all: cmsis usbcdc hal $(APPLICATION) link

cmsis:
	$(CC) $(CFLAGS) $(CMSIS_SRC)

usbcdc:
	$(CC) $(CFLAGS) $(USBCDC_SRC)
		
hal:
	$(CC) $(CFLAGS) $(HAL_SRC)

app:
	$(CC) $(CFLAGS) $(APP_SRC)
	
coos_stackless:
	$(CC) $(CFLAGS) -I $(COOS_STACKLESS_DIR) $(COOS_STACKLESS_SRC)
	$(CC) $(CFLAGS) -I $(COOS_STACKLESS_DIR) $(APP_SRC_STACKLESS)

coos_stackful:
	$(CC) $(CFLAGS) -I $(COOS_STACKFUL_DIR) $(COOS_STACKFUL_SRC)
	$(CC) $(CFLAGS) -I $(COOS_STACKFUL_DIR) $(APP_SRC_STACKFUL)
	
link:
	$(LD) $(LDFLAGS) -T$(LDSCRIPT) -Map $(PROG).map -o $(PROG).elf *.o 
	$(DUMP) --disassemble --reloc $(PROG).elf > $(PROG).lst	
	$(DUMP) -h $(PROG).elf > $(PROG).sec
	$(DUMP) -s $(PROG).elf > $(PROG).cnt
	$(OBJ) -O binary $(PROG).elf $(PROG).bin
	$(OBJ) -R .eeprom -O ihex $(PROG).elf $(PROG).hex
	$(SIZE) $(PROG).elf

flash:
	dfu-util -a 0 -s 0x08000000:leave -D $(PROG).bin

serial:
	stty -F ${SERIAL_DEV} ${SERIAL_BR} raw cs8 -echo
	cat ${SERIAL_DEV}

clean:
	rm -rf *.o *~ firmware.*
