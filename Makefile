TARGET1 = media_server

OBJS_PATH = objs/linux

CROSS_COMPILE =
CXX   = $(CROSS_COMPILE)g++
CC    = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip

INC  = -I$(shell pwd)/p2p/ 
INC += -I$(shell pwd)/p2p/asio
INC += -I$(shell pwd)/p2p/catch2
INC += -I$(shell pwd)/p2p/enet
INC += -I$(shell pwd)/p2p/fec
INC += -I$(shell pwd)/p2p/unittest
INC += -I$(shell pwd)/example/media_server

LIB  =

ASIO_FLAG = -DASIO_SEPARATE_COMPILATION

LD_FLAGS  = -lrt -pthread -lpthread -ldl -lm $(ASIO_FLAG)
CXX_FLAGS = -std=c++11

O_FLAG = -O2

SRC1  = $(notdir $(wildcard ./p2p/enet/*.c))
OBJS1 = $(patsubst %.c,$(OBJS_PATH)/%.o,$(SRC1))

SRC2  = $(notdir $(wildcard ./p2p/fec/*.c))
OBJS2 = $(patsubst %.c,$(OBJS_PATH)/%.o,$(SRC2))

SRC3  = $(notdir $(wildcard ./p2p/fec/*.cpp))
OBJS3 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC3))

SRC4  = $(notdir $(wildcard ./p2p/*.cpp))
OBJS4 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC4))

SRC5  = $(notdir $(wildcard ./example/media_server/*.cpp))
OBJS5 = $(patsubst %.cpp,$(OBJS_PATH)/%.o,$(SRC5))

all: BUILD_DIR $(TARGET1)

BUILD_DIR:
	@-mkdir -p $(OBJS_PATH)

$(TARGET1) : $(OBJS1) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5)
	$(CXX) $^ -o $@ $(LD_FLAGS) $(CXX_FLAGS)
    
$(OBJS_PATH)/%.o : ./p2p/enet/%.c
	$(CC) -c  $< -o  $@ $(INC)
$(OBJS_PATH)/%.o : ./p2p/fec/%.c
	$(CC) -c  $< -o  $@
$(OBJS_PATH)/%.o : ./p2p/fec/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) 
$(OBJS_PATH)/%.o : ./p2p/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
$(OBJS_PATH)/%.o : ./example/media_server/%.cpp
	$(CXX) -c  $< -o  $@  $(CXX_FLAGS) $(INC)
	
clean:
	-rm -rf $(OBJS_PATH) $(TARGET1)
