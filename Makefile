CC = g++
INC	= -I ./srcs/Config/ -I ./srcs/SocketInterface/ -I ./srcs/ApplicationServer/ \
		-I ./srcs/ApplicationServer/RequestParser/ -I ./srcs/ApplicationServer/StaticFileReader/ \
		-I ./srcs/ApplicationServer/DataProcessor
CFLAGS = -Wall -std=c++11 $(INC)
LDFLAGS =
SOURCES = srcs/main.cpp srcs/Config/Config.cpp srcs/SocketInterface/SocketInterface.cpp \
			srcs/ApplicationServer/ApplicationServer.cpp \
			srcs/Config/ConfigError.cpp srcs/Config/ConfigParser.cpp \
			srcs/Config/LocationContext.cpp srcs/Config/ServerContext.cpp \
			srcs/ApplicationServer/RequestParser/RequestParser.cpp \
			srcs/ApplicationServer/StaticFileReader/StaticFileReader.cpp \
			srcs/ApplicationServer/DataProcessor/DataProcessor.cpp
OBJECTS_DIR = objs
OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(SOURCES:.cpp=.o))
NAME = NeuroSrv

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJECTS_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJECTS_DIR)

fclean:
	rm -rf $(OBJECTS_DIR) $(NAME) ./post/*

re: fclean all