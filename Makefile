all:
	g++ main.cpp ./server/server.cpp ./client/client.cpp -lpthread -o main