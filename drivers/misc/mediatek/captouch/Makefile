include $(srctree)/drivers/misc/mediatek/Makefile.custom

# In case the platform does NOT support this type of sensors
obj-y   +=  captouch.o

ifeq ($(CONFIG_MTK_IQS128),y)
obj-y	+=  iqs128/
endif
