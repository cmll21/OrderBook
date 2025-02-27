CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -O2 -Ibackend/include
LIBS = -lboost_system -lboost_json

# Directories
SRC_DIR = backend/src
OBJ_DIR = backend/obj

# Source files
SRC_SERVER = $(SRC_DIR)/server.cpp $(SRC_DIR)/order_book.cpp $(SRC_DIR)/matching_engine.cpp $(SRC_DIR)/order.cpp $(SRC_DIR)/trade.cpp
SRC_CLIENT = $(SRC_DIR)/client.cpp
SRC_TESTER = $(SRC_DIR)/tester.cpp $(SRC_DIR)/order_book.cpp $(SRC_DIR)/matching_engine.cpp $(SRC_DIR)/order.cpp $(SRC_DIR)/trade.cpp

# Object files (automatically place .o in OBJ_DIR)
OBJ_SERVER = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_SERVER))
OBJ_CLIENT = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_CLIENT))
OBJ_TESTER = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_TESTER))

# Targets
TARGET_SERVER = order_server
TARGET_CLIENT = order_client
TARGET_TESTER = tester_app

all: $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_TESTER)

$(TARGET_SERVER): $(OBJ_SERVER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(TARGET_TESTER): $(OBJ_TESTER)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# Pattern rule for compiling .cpp to .o in OBJ_DIR
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_TESTER)

run_server:
	./$(TARGET_SERVER)

run_client:
	./$(TARGET_CLIENT)

run_tester:
	./$(TARGET_TESTER)
