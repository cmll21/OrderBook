CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2
LIBS = -lboost_system -lboost_json

SRC_SERVER = server.cpp order_book.cpp matching_engine.cpp order.cpp trade.cpp
SRC_CLIENT = client.cpp

OBJ_SERVER = $(SRC_SERVER:.cpp=.o)
OBJ_CLIENT = $(SRC_CLIENT:.cpp=.o)

TARGET_SERVER = order_server
TARGET_CLIENT = order_client

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJ_SERVER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERVER) $(OBJ_SERVER) $(LIBS)

$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(TARGET_CLIENT) $(OBJ_CLIENT) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_SERVER) $(OBJ_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)

run_server:
	./order_server

run_client:
	./order_client
