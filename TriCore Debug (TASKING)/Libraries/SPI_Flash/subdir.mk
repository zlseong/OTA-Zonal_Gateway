################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
"../Libraries/SPI_Flash/spi_flash_s25fl512s.c" 

COMPILED_SRCS += \
"Libraries/SPI_Flash/spi_flash_s25fl512s.src" 

C_DEPS += \
"./Libraries/SPI_Flash/spi_flash_s25fl512s.d" 

OBJS += \
"Libraries/SPI_Flash/spi_flash_s25fl512s.o" 


# Each subdirectory must supply rules for building sources it contributes
"Libraries/SPI_Flash/spi_flash_s25fl512s.src":"../Libraries/SPI_Flash/spi_flash_s25fl512s.c" "Libraries/SPI_Flash/subdir.mk"
	cctc -cs --dep-file="$*.d" --misrac-version=2004 -D__CPU__=tc37x "-fC:/Users/user/AURIX-v1.10.24-workspace/Zonal_Gateway/TriCore Debug (TASKING)/TASKING_C_C___Compiler-Include_paths__-I_.opt" --iso=99 --c++14 --language=+volatile --exceptions --anachronisms --fp-model=3 -O0 --tradeoff=4 --compact-max-size=200 -g -Wc-w544 -Wc-w557 -Wc-w508 -Ctc37x -Y0 -N0 -Z0 -o "$@" "$<"
"Libraries/SPI_Flash/spi_flash_s25fl512s.o":"Libraries/SPI_Flash/spi_flash_s25fl512s.src" "Libraries/SPI_Flash/subdir.mk"
	astc -Og -Os --no-warnings= --error-limit=42 -o  "$@" "$<"

clean: clean-Libraries-2f-SPI_Flash

clean-Libraries-2f-SPI_Flash:
	-$(RM) ./Libraries/SPI_Flash/spi_flash_s25fl512s.d ./Libraries/SPI_Flash/spi_flash_s25fl512s.o ./Libraries/SPI_Flash/spi_flash_s25fl512s.src

.PHONY: clean-Libraries-2f-SPI_Flash

