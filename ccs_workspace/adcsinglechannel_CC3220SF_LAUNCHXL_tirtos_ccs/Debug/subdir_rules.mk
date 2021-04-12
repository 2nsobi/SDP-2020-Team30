################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.2.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --include_path="C:/Users/Jonah/Desktop/Jonah/1Senior/SDP/SDP-2020-Team30/ccs_workspace/adcsinglechannel_CC3220SF_LAUNCHXL_tirtos_ccs" --include_path="C:/Users/Jonah/Desktop/Jonah/1Senior/SDP/SDP-2020-Team30/ccs_workspace/adcsinglechannel_CC3220SF_LAUNCHXL_tirtos_ccs/Debug" --include_path="C:/ti/simplelink_cc32xx_sdk_4_30_00_06/source" --include_path="C:/ti/simplelink_cc32xx_sdk_4_30_00_06/source/ti/posix/ccs" --include_path="C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.2.LTS/include" -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/Jonah/Desktop/Jonah/1Senior/SDP/SDP-2020-Team30/ccs_workspace/adcsinglechannel_CC3220SF_LAUNCHXL_tirtos_ccs/Debug/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-957620239: ../adcsinglechannel.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"C:/ti/ccs1011/ccs/utils/sysconfig_1.6.0/sysconfig_cli.bat" -s "C:/ti/simplelink_cc32xx_sdk_4_30_00_06/.metadata/product.json" -o "syscfg" --compiler ccs "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/ti_drivers_config.c: build-957620239 ../adcsinglechannel.syscfg
syscfg/ti_drivers_config.h: build-957620239
syscfg/ti_utils_build_linker.cmd.genlibs: build-957620239
syscfg/syscfg_c.rov.xs: build-957620239
syscfg/ti_utils_runtime_model.gv: build-957620239
syscfg/ti_utils_runtime_Makefile: build-957620239
syscfg/: build-957620239

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.2.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --include_path="C:/Users/Jonah/Desktop/Jonah/1Senior/SDP/SDP-2020-Team30/ccs_workspace/adcsinglechannel_CC3220SF_LAUNCHXL_tirtos_ccs" --include_path="C:/Users/Jonah/Desktop/Jonah/1Senior/SDP/SDP-2020-Team30/ccs_workspace/adcsinglechannel_CC3220SF_LAUNCHXL_tirtos_ccs/Debug" --include_path="C:/ti/simplelink_cc32xx_sdk_4_30_00_06/source" --include_path="C:/ti/simplelink_cc32xx_sdk_4_30_00_06/source/ti/posix/ccs" --include_path="C:/ti/ccs1011/ccs/tools/compiler/ti-cgt-arm_20.2.2.LTS/include" -g --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/Jonah/Desktop/Jonah/1Senior/SDP/SDP-2020-Team30/ccs_workspace/adcsinglechannel_CC3220SF_LAUNCHXL_tirtos_ccs/Debug/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


