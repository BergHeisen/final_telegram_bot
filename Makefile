MKDIR   := mkdir
RMDIR   := rm -rf
CC      := clang++
BIN     := ./bin
OBJ     := ./obj
TEST_OBJ := ./test_obj
INCLUDE := ./include
SRC     := ./src
TEST_SRC := ./test
LIBS		:= ./libs
SRCS    := $(wildcard $(SRC)/*.cpp)
SRCS_NOMAIN := $(filter-out ./src/main.cpp, $(wildcard $(SRC)/*.cpp))
TEST_SRCS    := $(wildcard $(TEST_SRC)/*.cpp)
TEST_OBJS := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS_NOMAIN)) $(patsubst $(TEST_SRC)/%.cpp,$(TEST_OBJ)/%.o,$(TEST_SRCS))

OBJS    := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))
EXE     := $(BIN)/main.out
TEST_EXE := $(BIN)/test.out
CFLAGS  := -DDebug -Wall -g -O0 -I$(INCLUDE) -I./third_party/tgbot-cpp/include -I./third_party/nlohmann_json/single_include -I./third_party/fmt/include -I./third_party/plog/include -I./third_party/thread-pool -std=c++17 -I./third_party/cpp_redis/includes
LDLIBS  := -lm -lssl -lcrypto -lpthread -lboost_system
STATIC_LIBRARIES := ${LIBS}/libTgBot.a ${LIBS}/libfmt.a ${LIBS}/libcpp_redis.a ${LIBS}/libtacopie.a

TEST_CFLAGS = $(CFLAGS)
TEST_LFLAGS = $(CFLAGS)
GTEST = /usr/local/lib/libgtest.a


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

$(BIN) $(OBJ) $(TEST_OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

$(TEST_OBJ)/%.o: $(TEST_SRC)/%.cpp | $(TEST_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_EXE): $(TEST_OBJS) 
	$(CC) $(LDFLAGS) $^ -o $@ $(GTEST) $(STATIC_LIBRARIES) $(LDLIBS)

test: $(TEST_EXE) 

all: ; $(info $$var is [${TEST_OBJS}])echo Hello world
clean:
	$(RMDIR) $(OBJ) $(BIN) $(LIBS) third_party/tgbot-cpp/build third_party/fmt/build third_party/cpp_redis/build

