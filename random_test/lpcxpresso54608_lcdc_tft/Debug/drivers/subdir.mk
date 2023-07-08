################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_emc.c \
../drivers/fsl_flexcomm.c \
../drivers/fsl_gpio.c \
../drivers/fsl_inputmux.c \
../drivers/fsl_lcdc.c \
../drivers/fsl_power.c \
../drivers/fsl_reset.c \
../drivers/fsl_sctimer.c \
../drivers/fsl_usart.c 

OBJS += \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_emc.o \
./drivers/fsl_flexcomm.o \
./drivers/fsl_gpio.o \
./drivers/fsl_inputmux.o \
./drivers/fsl_lcdc.o \
./drivers/fsl_power.o \
./drivers/fsl_reset.o \
./drivers/fsl_sctimer.o \
./drivers/fsl_usart.o 

C_DEPS += \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_emc.d \
./drivers/fsl_flexcomm.d \
./drivers/fsl_gpio.d \
./drivers/fsl_inputmux.d \
./drivers/fsl_lcdc.d \
./drivers/fsl_power.d \
./drivers/fsl_reset.d \
./drivers/fsl_sctimer.d \
./drivers/fsl_usart.d 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_LPC54608 -D__USE_CMSIS -DCPU_LPC54608J512ET180=1 -DSERIAL_PORT_TYPE_UART=1 -DCPU_LPC54608J512ET180_cm4 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -DDEBUG -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/board" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/source" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/drivers" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/device" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/utilities" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/component/uart" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/component/serial_manager" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/component/lists" -I"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/CMSIS" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


