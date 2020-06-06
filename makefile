CXX = g++
CPPFLAGS = -g -std=c++17 -Wall -Wextra -O2
LDFLAGS = -g

SERVER = ./Server/radioProxy.cc ./Server/messageServer.cc ./Server/outputManager.cc ./Server/parserServer.cc ./Server/serverManager.cc ./Server/shoutcastManager.cc ./Misc/err.cc ./Misc/message.cc ./Misc/parser.cc

CLIENT = ./Client/radioClient.cc ./Client/clientManager.cc ./Client/messageClient.cc ./Client/parserClient.cc ./Client/telnetManager.cc ./Client/userInterface.cc ./Misc/err.cc ./Misc/message.cc ./Misc/parser.cc

all: radio-proxy radio-client

radio-proxy: $(SERVER:%.cc=%.o)
	$(CXX) $(CPPFLAGS) -o radio-proxy $(SERVER:%.cc=%.o)

radio-client: $(CLIENT:%.cc=%.o)
	$(CXX) $(CPPFLAGS) -o radio-client $(CLIENT:%.cc=%.o)

clean:
	rm -f ./Misc/*.o
	rm -f ./Server/*.o
	rm -f ./Client/*.o
	rm radio-proxy
	rm radio-client
