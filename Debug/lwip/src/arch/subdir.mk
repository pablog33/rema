################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/arch/lpc18xx_43xx_emac.c \
../lwip/src/arch/lpc_debug.c \
../lwip/src/arch/sys_arch_freertos.c 

OBJS += \
./lwip/src/arch/lpc18xx_43xx_emac.o \
./lwip/src/arch/lpc_debug.o \
./lwip/src/arch/sys_arch_freertos.o 

C_DEPS += \
./lwip/src/arch/lpc18xx_43xx_emac.d \
./lwip/src/arch/lpc_debug.d \
./lwip/src/arch/sys_arch_freertos.d 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/arch/%.o: ../lwip/src/arch/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu17 -DDEBUG -DCORE_M4 -D__CODE_RED -D__USE_LPCOPEN -D__NEWLIB__ -I"/home/gspc/CIAA/rema/nfc/inc" -I"/home/gspc/CIAA/rema/freertos/inc" -I"/home/gspc/CIAA/rema/lwip/inc" -I"/home/gspc/CIAA/rema/lwip/inc/ipv4" -I"/home/gspc/CIAA/rema/freertos/inc" -I"/home/gspc/Proyectos/SM-13/CIAA_NXP_board/inc" -I"/home/gspc/Proyectos/SM-13/LpcChip/inc" -O3 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fsingle-precision-constant -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


