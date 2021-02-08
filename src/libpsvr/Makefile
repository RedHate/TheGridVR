#############################
# RedHate
#############################

PREFIX		=
TARGET		=  psvr
OBJ		=  libusb/core.o \
		   libusb/descriptor.o \
		   libusb/hotplug.o \
		   libusb/io.o \
		   libusb/linux_netlink.o \
		   libusb/linux_usbfs.o \
		   libusb/poll_posix.o \
		   libusb/strerror.o \
		   libusb/sync.o \
		   libusb/threads_posix.o \
		   psvr.o

PROGMAIN	=  main.o

CC		=  gcc
CXX		=  g++
LD		=  gcc
MV		=  mv
CP		=  cp
ECHO		=  echo
RM 		=  rm
AR		=  ar
RANLIB   	=  ranlib
STRIP		=  strip
PNG2H		=  tools/png2texh
OBJ2H		=  tools/obj2hGL

INCLUDES	?= -I/usr/include
LIBS		?= -L/usr/lib
LDFLAGS		?= -B/usr/lib
LDLIBS		?= -lpthread
CFLAGS   	?= -fPIC -O2 $(INCLUDES) $(LIBS)
CXXFLAGS 	?= -fPIC -O2 $(INCLUDES) $(LIBS)
WARNINGS	:= -w

## colors for fun
BLACK1		= \033[1;30m
BLACK2		= \033[0;30m
RED1		= \033[1;31m
RED2		= \033[0;31m
GREEN1		= \033[1;32m
GREEN2		= \033[0;32m
YELLOW1		= \033[1;33m
YELLOW		= \033[0;33m
BLUE1		= \033[1;34m
BLUE2		= \033[0;34m
PURPLE1		= \033[1;35m
PURPLE2		= \033[0;35m
CYAN1		= \033[1;36m
CYAN2		= \033[0;36m
WHITE1		= \033[1;37m
WHITE2		= \033[0;37m
NOCOLOR		= \033[0m

.PHONY: all run clean

all: $(ASSETS) $(OBJ) $(PROGMAIN) $(RES) $(TARGET) libpsvr.a libpsvr.so
#$(TARGET_LIB).so

run:  $(ASSETS) $(OBJ) $(RES) $(TARGET) libpsvr.a
	@./$(TARGET)

clean:
	@printf "$(RED1)[CLEANING]$(NOCOLOR)\n"
	@rm $(OBJ) $(RES) $(TARGET) $(ASSETS) libpsvr.a libpsvr.so

%.h: %.obj
	@printf "$(RED1)[OBJ2H]$(NOCOLOR) $(notdir $(basename $<)).h\n"
	@cp $(basename $<).obj $(notdir $(basename $<)).obj
	@cp $(basename $<).mtl $(notdir $(basename $<)).mtl
	@$(OBJ2H) $(notdir $<) $(notdir $(basename $<))
	@rm $(notdir $(basename $<)).obj $(notdir $(basename $<)).mtl
	@mv $(notdir $(basename $<)).h $(MODELS)/$(notdir $(basename $<)).h

%.h: %.png
	@printf "$(RED1)[PNG2H]$(NOCOLOR) $(notdir $(basename $<)).h\n"
	@$(PNG2H) $< $(notdir $(basename $<))
	@mv $(notdir $(basename $<)).h $(TEXTURES)/$(notdir $(basename $<)).h

%.o: %.cpp
	@printf "$(RED1)[CXX]$(NOCOLOR) $(notdir $(basename $<)).o\n"
	@$(CXX) $(WARNINGS) -c $< $(CXXFLAGS) -o $(basename $<).o

%.o: %.cxx
	@printf "$(RED1)[CXX]$(NOCOLOR) $(notdir $(basename $<)).o\n"
	@$(CXX) $(WARNINGS) -c $< $(CFLAGS) -o $(basename $<).o

%.o: %.c
	@printf "$(RED1)[CC]$(NOCOLOR) $(notdir $(basename $<)).o\n"
	@$(CC) $(WARNINGS) -c $< $(CFLAGS) -o $(basename $<).o

libpsvr.a:
	@printf "$(RED1)[CC]$(NOCOLOR) libpsvr.a\n"
	@$(AR) -crU libpsvr.a $(OBJ)

libpsvr.so:
	@printf "$(RED1)[CC]$(NOCOLOR) libpsvr.so\n"
	@$(CC) $(OBJ) $(CFLAGS)  -shared -o libpsvr.so

$(TARGET): $(ASSETS) $(OBJ)
	@printf "$(RED1)[CC]$(NOCOLOR) $(TARGET)\n"
	@$(CC) $(PROGMAIN) $(OBJ) -static $(LDLIBS) $(LDFLAGS) $(CXXFLAGS) -o $(TARGET)
	@$(STRIP) -s $(TARGET)
