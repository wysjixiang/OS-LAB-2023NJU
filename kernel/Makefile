NAME := pmm
SRCS := $(wildcard src/*.c)
SRCS += $(wildcard framework/*.c)


export MODULE  := L1
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
#export ARCH := x86_64-qemu
export ARCH := native
endif

include $(AM_HOME)/Makefile
#include ../Makefile.lab
#image: git
