NAME := kmt
SRCS := $(wildcard src/*.c)
SRCS += $(wildcard src/dev/*.c)
SRCS += $(wildcard framework/*.c)
#CFLAGS += -D__MEM_TEST
CFLAGS += -DNO_DEBUG

export MODULE  := L2
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
#export ARCH := x86_64-qemu
export ARCH := native
endif

include $(AM_HOME)/Makefile
#include ../Makefile.lab
#image: git
