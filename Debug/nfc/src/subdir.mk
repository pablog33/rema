################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../nfc/src/cr_startup_lpc43xx.c \
../nfc/src/crp.c \
../nfc/src/debug.c \
../nfc/src/eeprom.c \
../nfc/src/encoders.c \
../nfc/src/gpio.c \
../nfc/src/json_wp.c \
../nfc/src/lwip_init.c \
../nfc/src/main.c \
../nfc/src/mem_check.c \
../nfc/src/mot_pap.c \
../nfc/src/net_commands.c \
../nfc/src/one-wire.c \
../nfc/src/parson.c \
../nfc/src/relay.c \
../nfc/src/settings.c \
../nfc/src/sysinit.c \
../nfc/src/tcp_server.c \
../nfc/src/temperature.c \
../nfc/src/temperature_ds18b20.c \
../nfc/src/tmr.c \
../nfc/src/wait.c \
../nfc/src/x_axis.c \
../nfc/src/x_zs.c \
../nfc/src/y_axis.c \
../nfc/src/z_axis.c 

OBJS += \
./nfc/src/cr_startup_lpc43xx.o \
./nfc/src/crp.o \
./nfc/src/debug.o \
./nfc/src/eeprom.o \
./nfc/src/encoders.o \
./nfc/src/gpio.o \
./nfc/src/json_wp.o \
./nfc/src/lwip_init.o \
./nfc/src/main.o \
./nfc/src/mem_check.o \
./nfc/src/mot_pap.o \
./nfc/src/net_commands.o \
./nfc/src/one-wire.o \
./nfc/src/parson.o \
./nfc/src/relay.o \
./nfc/src/settings.o \
./nfc/src/sysinit.o \
./nfc/src/tcp_server.o \
./nfc/src/temperature.o \
./nfc/src/temperature_ds18b20.o \
./nfc/src/tmr.o \
./nfc/src/wait.o \
./nfc/src/x_axis.o \
./nfc/src/x_zs.o \
./nfc/src/y_axis.o \
./nfc/src/z_axis.o 

C_DEPS += \
./nfc/src/cr_startup_lpc43xx.d \
./nfc/src/crp.d \
./nfc/src/debug.d \
./nfc/src/eeprom.d \
./nfc/src/encoders.d \
./nfc/src/gpio.d \
./nfc/src/json_wp.d \
./nfc/src/lwip_init.d \
./nfc/src/main.d \
./nfc/src/mem_check.d \
./nfc/src/mot_pap.d \
./nfc/src/net_commands.d \
./nfc/src/one-wire.d \
./nfc/src/parson.d \
./nfc/src/relay.d \
./nfc/src/settings.d \
./nfc/src/sysinit.d \
./nfc/src/tcp_server.d \
./nfc/src/temperature.d \
./nfc/src/temperature_ds18b20.d \
./nfc/src/tmr.d \
./nfc/src/wait.d \
./nfc/src/x_axis.d \
./nfc/src/x_zs.d \
./nfc/src/y_axis.d \
./nfc/src/z_axis.d 


# Each subdirectory must supply rules for building sources it contributes
nfc/src/%.o: ../nfc/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu17 -DDEBUG -DCORE_M4 -D__CODE_RED -D__USE_LPCOPEN -D__NEWLIB__ -I"/home/gspc/CIAA/rema/nfc/inc" -I"/home/gspc/CIAA/rema/freertos/inc" -I"/home/gspc/CIAA/rema/lwip/inc" -I"/home/gspc/CIAA/rema/lwip/inc/ipv4" -I"/home/gspc/CIAA/rema/freertos/inc" -I"/home/gspc/Proyectos/SM-13/CIAA_NXP_board/inc" -I"/home/gspc/Proyectos/SM-13/LpcChip/inc" -O3 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fsingle-precision-constant -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


