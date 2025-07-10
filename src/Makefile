CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS  := -lssl -lcrypto

SRC_DIR  := merkle
SRCS     := $(SRC_DIR)/handler.cpp \
            $(SRC_DIR)/merkleTree.cpp \
            $(SRC_DIR)/utils.cpp \
            $(SRC_DIR)/merkleNode.cpp

TARGET   := $(SRC_DIR)/mtfs

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)