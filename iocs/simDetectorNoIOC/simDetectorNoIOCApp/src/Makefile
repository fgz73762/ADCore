TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# The following gets rid of the -fno-implicit-templates flag on vxWorks, 
# so we get automatic template instantiation.
# This is what we want for miscellaneous/asynPortDriver.cpp
ifeq (vxWorks,$(findstring vxWorks, $(T_A)))
CODE_CXXFLAGS=
endif

PROD_IOC += simDetectorNoIOCApp
PROD_SRCS += simDetectorNoIOC.cpp

PROD_LIBS += simDetector
PROD_LIBS += ADBase
PROD_LIBS += NDPlugin
PROD_LIBS += asyn
PROD_LIBS += Com

#include $(ADCORE)/ADApp/commonDriverMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

