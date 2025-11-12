################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/Ethernet/lwip_isr.c" 

COMPILED_SRCS += \
"Libraries/Ethernet/lwip_isr.src" 

C_DEPS += \
"./Libraries/Ethernet/lwip_isr.d" 

OBJS += \
"Libraries/Ethernet/lwip_isr.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/Ethernet/lwip_isr.src":"../Libraries/Ethernet/lwip_isr.c" "Libraries/Ethernet/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Ethernet/lwip_isr.o":"Libraries/Ethernet/lwip_isr.src" "Libraries/Ethernet/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-Ethernet

clean-Libraries-2f-Ethernet:
	-$(RM) ./Libraries/Ethernet/lwip_isr.d ./Libraries/Ethernet/lwip_isr.o ./Libraries/Ethernet/lwip_isr.src

.PHONY: clean-Libraries-2f-Ethernet

