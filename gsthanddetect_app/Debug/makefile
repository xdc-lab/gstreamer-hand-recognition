################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: gst_handdetect_plugin

# Tool invocations
gst_handdetect_plugin: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L/usr/lib -L/usr/local/lib -L/usr/local/lib/gstreamer-0.10 -L/usr/local/lib/glib-2.0 -L/usr/lib/gstreamer-0.10 -o "gst_handdetect_plugin" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) gst_handdetect_plugin
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
