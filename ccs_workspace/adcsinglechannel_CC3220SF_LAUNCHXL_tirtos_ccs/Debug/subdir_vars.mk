################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../cc32xxsf_tirtos.cmd 

SYSCFG_SRCS += \
../adcsinglechannel.syscfg 

C_SRCS += \
../adcsinglechannel.c \
./syscfg/ti_drivers_config.c \
../main_tirtos.c 

GEN_FILES += \
./syscfg/ti_drivers_config.c 

GEN_MISC_DIRS += \
./syscfg/ 

C_DEPS += \
./adcsinglechannel.d \
./syscfg/ti_drivers_config.d \
./main_tirtos.d 

OBJS += \
./adcsinglechannel.obj \
./syscfg/ti_drivers_config.obj \
./main_tirtos.obj 

GEN_MISC_FILES += \
./syscfg/ti_drivers_config.h \
./syscfg/ti_utils_build_linker.cmd.genlibs \
./syscfg/syscfg_c.rov.xs \
./syscfg/ti_utils_runtime_model.gv \
./syscfg/ti_utils_runtime_Makefile 

GEN_MISC_DIRS__QUOTED += \
"syscfg\" 

OBJS__QUOTED += \
"adcsinglechannel.obj" \
"syscfg\ti_drivers_config.obj" \
"main_tirtos.obj" 

GEN_MISC_FILES__QUOTED += \
"syscfg\ti_drivers_config.h" \
"syscfg\ti_utils_build_linker.cmd.genlibs" \
"syscfg\syscfg_c.rov.xs" \
"syscfg\ti_utils_runtime_model.gv" \
"syscfg\ti_utils_runtime_Makefile" 

C_DEPS__QUOTED += \
"adcsinglechannel.d" \
"syscfg\ti_drivers_config.d" \
"main_tirtos.d" 

GEN_FILES__QUOTED += \
"syscfg\ti_drivers_config.c" 

C_SRCS__QUOTED += \
"../adcsinglechannel.c" \
"./syscfg/ti_drivers_config.c" \
"../main_tirtos.c" 

SYSCFG_SRCS__QUOTED += \
"../adcsinglechannel.syscfg" 


