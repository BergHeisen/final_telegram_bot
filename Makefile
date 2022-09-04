MKDIR   := mkdir
RMDIR   := rm -rf
CC      := g++
BIN     := ./bin
OBJ     := ./obj
INCLUDE := ./include
SRC     := ./src
LIBS		:= ./libs
SRCS    := $(wildcard $(SRC)/*.cpp)
OBJS    := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))
EXE     := $(BIN)/main.out
CFLAGS  := -g3 -I$(INCLUDE) -I./third_party/tgbot-cpp/include -I./third_party/nlohmann_json/single_include
LDLIBS  := -lm -lssl -lcrypto -lpthread -lboost_system
STATIC_LIBRARIES := ${LIBS}/libTgBot.a

.PHONY: all run clean

all: $(EXE)

$(LIBS)/libTgBot.a:
	cd third_party/tgbot-cpp; \
		cmake .
	cd ../..
	make -j16 -C third_party/tgbot-cpp
	mkdir -p $(LIBS)
	cp third_party/tgbot-cpp/libTgBot.a $(LIBS)

$(EXE): $(OBJS) ${STATIC_LIBRARIES} | $(BIN) 
	$(CC) $(LDFLAGS) $^ -o $@ $(STATIC_LIBRARIES) $(LDLIBS) 

$(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

clean:
	$(RMDIR) $(OBJ) $(BIN) $(LIBS)

