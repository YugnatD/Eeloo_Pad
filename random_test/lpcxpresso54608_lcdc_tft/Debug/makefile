################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/lists/subdir.mk
-include board/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: lpcxpresso54608_lcdc_tft.axf

# Tool invocations
lpcxpresso54608_lcdc_tft.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/home/tanguy/Documents/Eeloo_Pad/random_test/lpcxpresso54608_lcdc_tft/libs" -Xlinker -Map="lpcxpresso54608_lcdc_tft.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -T lpcxpresso54608_lcdc_tft_Debug.ld -o "lpcxpresso54608_lcdc_tft.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) lpcxpresso54608_lcdc_tft.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "lpcxpresso54608_lcdc_tft.axf"; # arm-none-eabi-objcopy -v -O binary "lpcxpresso54608_lcdc_tft.axf" "lpcxpresso54608_lcdc_tft.bin" ; # checksum -p LPC54608J512 -d "lpcxpresso54608_lcdc_tft.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
