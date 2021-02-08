#############################
# RedHate
#############################

#WITHGLES	:= $(shell whereis libGLESv1_CM.so |basename -s `grep /`)

UNAME		:= $(shell uname)
DATE		:= `date`
WHOAMI		:= `whoami`
CURTIME		:= $(date)

TARGET		=  TheGrid

OBJ			=  src/main.o

LDLIBS		=

ifeq ($(UNAME), Linux)
ifeq ($(WITHGLES), libGLESv1_CM.so)
LDLIBS		=  -lGLESv1_CM
TARGET		:=  $(TARGET)-gles
else
LDLIBS		=  -lGL
TARGET		:=  $(TARGET)-gl1
endif
LDLIBS		+=   -lX11 -lSDL2main -lSDL2 -lpng -lz -lm -pthread -lXrandr src/libpsvr/libpsvr.so
endif

##windows
#ifeq ($(UNAME), Windows)
#LDLIBS		=  -lmingw32 -mwindows -mconsole -lopengl32 -lSDL2main -lSDL2 -lwinmm -lpng -lz -lm -lole32 -loleaut32 -limm32 -lversion
#endif

CC			=  gcc
CXX			=  g++
LD			=  gcc
MV			=  mv
CP			=  cp
ECHO		=  echo
RM 			=  rm
AR			=  ar
RANLIB   	=  ranlib
STRIP		=  strip
PNG2H		=  tools/png2texh
OBJ2H		=  tools/obj2hGL

INCLUDES	?= -I/usr/include
LIBS		?= -L/usr/lib

CFLAGS   	= -Wall -g -O2 $(INCLUDES) $(LIBS) -fPIC -no-pie
CXXFLAGS 	= -Wall -g -O2 $(INCLUDES) $(LIBS) -fPIC -no-pie

ifeq ($(WITHGLES), libGLESv1_CM.so)
CFLAGS   	+= -D_WITH_LIBGLES
CXXFLAGS 	+= -D_WITH_LIBGLES
endif

WARNINGS	:= -w

## colors for fun
RED1		= \033[1;31m
RED2		= \033[0;31m
NOCOLOR		= \033[0m

.PHONY: all run clean

all: $(ASSETS) $(OBJ) $(TARGET) $(TARGET_LIB)

run:  $(ASSETS) $(OBJ) $(TARGET) $(TARGET_LIB)
	@./$(TARGET)

clean:
	@printf "$(RED1)[CLEANING]$(NOCOLOR)\n"
	@rm $(OBJ) $(TARGET) $(TARGET_LIB) $(ASSETS)

install:
	@cp -Rp $(TARGET) /usr/bin/$(TARGET)
	@cp -Rp src/libpsvr/libpsvr.so /usr/lib

uninstall:
	@rm -rf /usr/bin/$(TARGET)
	@rm -rf /usr/lib/libpsvr.so

list:
	@echo $(INCLUDES) $(LIBS)

%.o: %.cpp
	@printf "$(RED1)[CXX]$(NOCOLOR) $(notdir $(basename $<)).o\n"
	@$(CXX) $(WARNINGS) -c $< $(CXXFLAGS) -o $(basename $<).o

%.o: %.cxx
	@printf "$(RED1)[CXX]$(NOCOLOR) $(notdir $(basename $<)).o\n"
	@$(CXX) $(WARNINGS) -c $< $(CFLAGS) -o $(basename $<).o

%.o: %.c
	@printf "$(RED1)[CC]$(NOCOLOR) $(notdir $(basename $<)).o\n"
	@$(CC) $(WARNINGS) -c $< $(CFLAGS) -o $(basename $<).o

%.a:
	@printf "$(RED1)[CC]$(NOCOLOR) $(basename $(TARGET_LIB)).a\n"
	@$(AR) -cru $(basename $(TARGET_LIB)).a $(OBJ)

$(TARGET): $(ASSETS) $(OBJ)
	@printf "$(RED1)[CXX]$(NOCOLOR) $(TARGET) - Platform: $(UNAME)\n"
	@$(CXX) $(WARNINGS)  $(OBJ) -o $(TARGET) $(CXXFLAGS) $(LDLIBS) `sdl2-config --cflags --libs` -Wl,-E
	@chmod a+x $(TARGET)
	@$(STRIP) -s $(TARGET)
