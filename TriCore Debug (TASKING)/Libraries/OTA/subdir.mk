################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/OTA/ota_manager.c" 

COMPILED_SRCS += \
"Libraries/OTA/ota_manager.src" 

C_DEPS += \
"./Libraries/OTA/ota_manager.d" 

OBJS += \
"Libraries/OTA/ota_manager.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/OTA/ota_manager.src":"../Libraries/OTA/ota_manager.c" "Libraries/OTA/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/Zonal_Gateway/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/OTA/ota_manager.o":"Libraries/OTA/ota_manager.src" "Libraries/OTA/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-OTA

clean-Libraries-2f-OTA:
	-$(RM) ./Libraries/OTA/ota_manager.d ./Libraries/OTA/ota_manager.o ./Libraries/OTA/ota_manager.src

.PHONY: clean-Libraries-2f-OTA

