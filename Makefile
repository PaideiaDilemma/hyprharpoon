all:
	$(CXX) -shared -fPIC --no-gnu-unique main.cpp indicators.cpp -o hyprharpoon.so -g `pkg-config --cflags pixman-1 hyprland libdrm pangocairo` -std=c++2b -O2
clean:
	rm ./borders-plus-plus.so
