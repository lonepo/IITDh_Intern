################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"/home/asus/ti/ccs2041/ccs/tools/compiler/ti-cgt-c2000_22.6.3.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --cla_support=cla1 --tmu_support=tmu0 --vcu_support=vcu2 --include_path="/home/asus/ti/ccs2041/ccs/tools/compiler/ti-cgt-c2000_22.6.3.LTS/include" --include_path="/home/asus/ti/c2000/C2000Ware_6_00_01_00/device_support/f2837xd/headers/include" --include_path="/home/asus/ti/c2000/C2000Ware_6_00_01_00/device_support/f2837xd/common/include" -g --define=CPU1 --display_error_number --diag_suppress=10063 --diag_warning=225 --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"/home/asus/ti/ccs2041/ccs/tools/compiler/ti-cgt-c2000_22.6.3.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --cla_support=cla1 --tmu_support=tmu0 --vcu_support=vcu2 --include_path="/home/asus/ti/ccs2041/ccs/tools/compiler/ti-cgt-c2000_22.6.3.LTS/include" --include_path="/home/asus/ti/c2000/C2000Ware_6_00_01_00/device_support/f2837xd/headers/include" --include_path="/home/asus/ti/c2000/C2000Ware_6_00_01_00/device_support/f2837xd/common/include" -g --define=CPU1 --display_error_number --diag_suppress=10063 --diag_warning=225 --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


