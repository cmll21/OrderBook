CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2
LIBS = -lboost_system -lboost_json

SRC_SERVER = server.cpp order_book.cpp matching_engine.cpp order.cpp trade.cpp
SRC_CLIENT = client.cpp
SRC_TESTER = tester.cpp order_book.cpp matching_engine.cpp order.cpp trade.cpp

OBJ_SERVER = $(SRC_SERVER:.cpp=.o)
OBJ_CLIENT = $(SRC_CLIENT:.cpp=.o)
OBJ_TESTER = tester.o order_book.o matching_engine.o order.o trade.o

TARGET_SERVER = server
TARGET_CLIENT = client
TARGET_TESTER = tester

all: $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_TESTER)

$(TARGET_SERVER): $(OBJ_SERVER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERVER) $(OBJ_SERVER) $(LIBS)

$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(TARGET_CLIENT) $(OBJ_CLIENT) $(LIBS)

$(TARGET_TESTER): $(OBJ_TESTER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_TESTER) $(OBJ_TESTER) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_SERVER) $(OBJ_CLIENT) $(OBJ_TESTER) $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_TESTER)

run_server:
	./$(TARGET_SERVER)

run_client:
	./$(TARGET_CLIENT)

run_tester:
	./$(TARGET_TESTER)
