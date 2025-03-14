# 编译器
CXX := g++

# 编译选项
CXXFLAGS := -g -Wall -std=c++17

# 目录定义
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
INCLUDE_DIR := include
LIB_DIR := lib

# 创建必要的目录
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# 第三方库包含路径（根据 vcpkg 安装路径调整）
INCLUDE_DIRS := -I$(INCLUDE_DIR) -I/home/vijaysue/vcpkg/installed/x64-linux/include

# 第三方库链接路径（根据 vcpkg 安装路径调整）
LIBRARY_DIRS := -L$(LIB_DIR) -L/home/vijaysue/vcpkg/installed/x64-linux/lib

# 需要链接的第三方库（添加 OpenSSL 依赖）
LIBRARIES := -lspdlog -lcpprest -lpthread -lssl -lcrypto

# 源文件列表
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)

# 生成对应的目标文件列表
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# 目标可执行文件名称
TARGET := $(BIN_DIR)/stability_server

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@ $(LIBRARY_DIRS) $(LIBRARIES) -lmodbus

# 编译每个 .cpp 文件为目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# 清理生成的文件
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

# 伪目标声明
.PHONY: all clean