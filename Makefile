TARGET = speakerfix_game
OBJS = main.o

BUILD_PRX = 1

CFLAGS = -O2 -G0 -Wall -I/ark/common/include
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

LIBDIR = /ark/libs/SystemCtrlForUser
LIBS = -lpspsystemctrl_user

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak