################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/Flash4/Flash4_Driver.c" \
"../Libraries/Flash4/Flash4_Test.c" 

COMPILED_SRCS += \
"Libraries/Flash4/Flash4_Driver.src" \
"Libraries/Flash4/Flash4_Test.src" 

C_DEPS += \
"./Libraries/Flash4/Flash4_Driver.d" \
"./Libraries/Flash4/Flash4_Test.d" 

OBJS += \
"Libraries/Flash4/Flash4_Driver.o" \
"Libraries/Flash4/Flash4_Test.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/Flash4/Flash4_Driver.src":"../Libraries/Flash4/Flash4_Driver.c" "Libraries/Flash4/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Flash4/Flash4_Driver.o":"Libraries/Flash4/Flash4_Driver.src" "Libraries/Flash4/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"
"Libraries/Flash4/Flash4_Test.src":"../Libraries/Flash4/Flash4_Test.c" "Libraries/Flash4/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/OTA-Zonal_Gateway/OTA-Zonal_Gateway-main/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/Flash4/Flash4_Test.o":"Libraries/Flash4/Flash4_Test.src" "Libraries/Flash4/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-Flash4

clean-Libraries-2f-Flash4:
	-$(RM) ./Libraries/Flash4/Flash4_Driver.d ./Libraries/Flash4/Flash4_Driver.o ./Libraries/Flash4/Flash4_Driver.src ./Libraries/Flash4/Flash4_Test.d ./Libraries/Flash4/Flash4_Test.o ./Libraries/Flash4/Flash4_Test.src

.PHONY: clean-Libraries-2f-Flash4

