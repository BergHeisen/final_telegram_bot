MKDIR   := mkdir
RMDIR   := rm -rf
CC      := clang++
BIN     := ./bin
OBJ     := ./obj
INCLUDE := ./include
SRC     := ./src
LIBS		:= ./libs
SRCS    := $(wildcard $(SRC)/*.cpp)
OBJS    := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))
EXE     := $(BIN)/main.out
CFLAGS  := -DDebug -Wall -g -O0 -I$(INCLUDE) -I./third_party/tgbot-cpp/include -I./third_party/nlohmann_json/single_include -I./third_party/fmt/include -I./third_party/plog/include -I./third_party/thread-pool -std=c++17 -I/opt/homebrew/Cellar/boost/1.79.0_1/include/ -I./third_party/cpp_redis/includes
LDLIBS  := -lm -lssl -lcrypto -lpthread -lboost_system
STATIC_LIBRARIES := ${LIBS}/libTgBot.a ${LIBS}/libfmt.a ${LIBS}/libcpp_redis.a ${LIBS}/libtacopie.a

.PHONY: all run clean

all: $(EXE)

$(LIBS)/libcpp_redis.a:
	mkdir -p third_party/cpp_redis/build
	cd third_party/cpp_redis/build; \
		cmake ..; \
		make -j16 
	mkdir -p $(LIBS)
	cp third_party/cpp_redis/build/lib/libcpp_redis.a $(LIBS)

$(LIBS)/libtacopie.a: $(LIBS)/libcpp_redis.a
	cp third_party/cpp_redis/build/lib/libtacopie.a $(LIBS)



$(LIBS)/libfmt.a:
	mkdir -p third_party/fmt/build
	cd third_party/fmt/build; \
		cmake ..; \
		make -j16 fmt
	mkdir -p $(LIBS)
	cp third_party/fmt/build/libfmt.a $(LIBS)

$(LIBS)/libTgBot.a:
	mkdir -p third_party/tgbot-cpp/build
	cd third_party/tgbot-cpp/build; \
		cmake ..; \
		make -j16
	mkdir -p $(LIBS)
	cp third_party/tgbot-cpp/build/libTgBot.a $(LIBS)

$(EXE): $(OBJS) ${STATIC_LIBRARIES} | $(BIN) 
	$(CC) $(LDFLAGS) $^ -o $@ $(STATIC_LIBRARIES) $(LDLIBS) 

$(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

clean:
	$(RMDIR) $(OBJ) $(BIN) $(LIBS) third_party/tgbot-cpp/build third_party/fmt/build third_party/cpp_redis/build

