#---------------------------------------------------------------------------------
# Makefile for NightFox's Lib Projects
#---------------------------------------------------------------------------------

export DEVKITPRO ?= /opt/devkitpro
export DEVKITARM ?= $(DEVKITPRO)/devkitARM
export PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)

#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif


#---------------------------------------------------------------------------------
# This part substitutes this include:
# include $(DEVKITARM)/ds_rules
# This allows you to set ROM info and icon easy
# Please update this block from DS_RULES file at every DEVKITARM update
#---------------------------------------------------------------------------------
include $(DEVKITARM)/base_rules
