# 编译器
CXX := g++

# 编译选项
CXXFLAGS := -g -Wall -std=c++17 -static-libgcc -static-libstdc++

# 降低内存使用的编译选项
CXXFLAGS += --param ggc-min-expand=10 --param ggc-min-heapsize=8192

# 目录定义
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
INCLUDE_DIR := include
LIB_DIR := lib

# 第三方库包含路径（根据 vcpkg 安装路径调整）
INCLUDE_DIRS := -I$(INCLUDE_DIR) -I/home/vijaysue/vcpkg/installed/x64-linux/include -I/usr/include/snap7

# 第三方库链接路径（根据 vcpkg 安装路径调整）
LIBRARY_DIRS := -L$(LIB_DIR) -L/home/vijaysue/vcpkg/installed/x64-linux/lib -L/usr/lib

# 需要链接的第三方（添加 OpenSSL 依赖）
# 静态链接选项
LIBRARIES := -Wl,-Bstatic -lspdlog -lcpprest -Wl,-Bdynamic -lpthread -lssl -lcrypto -lsnap7

# 源文件列表
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)

# 生成对应的目标文件列表
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# 目标可执行文件名称
TARGET := $(BIN_DIR)/stability_server

# 全静态编译选项（可选，取消注释以启用完全静态编译）
# 注意：某些库可能不支持完全静态链接，特别是SSL和系统库
#CXXFLAGS += -static
#LIBRARIES := -static -lspdlog -lcpprest -lpthread -lssl -lcrypto -lsnap7

# 默认目标
all: $(TARGET)

# 链接目标文件生成可执行文件
$(TARGET): $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@ $(LIBRARY_DIRS) $(LIBRARIES)

# 编译每个 .cpp 文件为目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@ 

# 单独编译特别大的文件，使用更保守的内存设置
$(OBJ_DIR)/server.o: $(SRC_DIR)/server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@ -O0

# 全静态版本目标（可选）
static: CXXFLAGS += -static
static: LIBRARIES := -static -lspdlog -lcpprest -lpthread -lssl -lcrypto -lsnap7
static: $(TARGET)

# 清理生成的文件
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

# 伪目标声明
.PHONY: all clean static