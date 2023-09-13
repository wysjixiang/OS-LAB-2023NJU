NAME := $(shell basename $(PWD))
export MODULE := M1
all: $(NAME)-64 $(NAME)-32
export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
export ARCH := native
endif


include ../Makefile
include $(AM_HOME)/Makefile