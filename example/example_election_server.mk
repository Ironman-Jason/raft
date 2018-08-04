app=example_election_server

SOURCES=example_election_server.cpp

OBJECTS=$(SOURCES:.cpp=.o)

all: $(app)

DEBUG=-g
INCLUDES=-I/usr/include/ -I/usr/include/boost/asio -I../ -I./
OPT=-O3
DEFINES=-DBOOST_LOG_DYN_LINK
LIBS=-pthread -lboost_log -lboost_log_setup -lboost_thread -lboost_system

CXXFLAGS=-std=c++17 -MD -O3 -Wall -Wextra $(DEFINES) $(INCLUDES) $(OPT) $(DEBUG)
CXXLINKS=$(CXXFLAGS) $(LIBS)

COMPILER=clang++

$(app): %: %.o $(OBJECTS)
	$(COMPILER) $(CXXLINKS) $^ -o $@

%.o: %.cpp
	$(COMPILER) $(CXXFLAGS) -c $^

-include $(SOURCES:=.d) $(apps:=.d)

clean:
	-rm $(app) *.o *.d 2> /dev/null
