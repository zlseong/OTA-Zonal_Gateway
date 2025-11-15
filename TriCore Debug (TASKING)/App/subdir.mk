################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../App/SystemInit.c" \
"../App/SystemMain.c" 

COMPILED_SRCS += \
"App/SystemInit.src" \
"App/SystemMain.src" 

C_DEPS += \
"./App/SystemInit.d" \
"./App/SystemMain.d" 

OBJS += \
"App/SystemInit.o" \
"App/SystemMain.o" 


# Each subdirectory must supply rules for building sources it contributes
"App/SystemInit.src":"../App/SystemInit.c" "App/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"App/SystemInit.o":"App/SystemInit.src" "App/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"App/SystemMain.src":"../App/SystemMain.c" "App/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"App/SystemMain.o":"App/SystemMain.src" "App/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-App

clean-App:
	-$(RM) ./App/SystemInit.d ./App/SystemInit.o ./App/SystemInit.src ./App/SystemMain.d ./App/SystemMain.o ./App/SystemMain.src

.PHONY: clean-App


