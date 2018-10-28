.PHONY: all cmake_conf create_build_dir

BUILD_DIR=.build

all: cmake_conf
	make -C $(BUILD_DIR)

cmake_conf: CMakeLists.txt $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
