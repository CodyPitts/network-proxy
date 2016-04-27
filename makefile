all: proxy

proxy: proxy.cpp 
	g++  -lpthread proxy.cpp

