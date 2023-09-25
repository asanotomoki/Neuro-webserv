NAME := webserv
CC = g++
INC	= -I ./srcs/Config/ -I ./srcs/SocketInterface/ -I ./srcs/CoreHandler/ \
		-I ./srcs/CoreHandler/RequestParser/ -I ./srcs/CoreHandler/StaticFileReader/ \
		-I ./srcs/CoreHandler/DataProcessor -I ./srcs/CoreHandler/Cgi/
CFLAGS = -Wall -Wextra -Werror -std=c++98 $(INC)
LDFLAGS =
SOURCES = srcs/main.cpp srcs/Config/Config.cpp srcs/SocketInterface/SocketInterface.cpp \
			srcs/CoreHandler/CoreHandler.cpp \
			srcs/CoreHandler/ParseRequestUrl.cpp \
			srcs/Config/ConfigError.cpp srcs/Config/ConfigParser.cpp \
			srcs/Config/CGIContext.cpp \
			srcs/Config/LocationContext.cpp srcs/Config/ServerContext.cpp \
			srcs/CoreHandler/RequestParser/RequestParser.cpp \
			srcs/CoreHandler/StaticFileReader/StaticFileReader.cpp \
			srcs/CoreHandler/DataProcessor/DataProcessor.cpp \
			srcs/CoreHandler/Cgi/Cgi.cpp
OBJECTS_DIR = objs
OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(SOURCES:.cpp=.o))

all: $(NAME)

test: 
	$(CC) $(TEST_CFLAGS) $(SOURCES) -o $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJECTS_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJECTS_DIR)

fclean:
	rm -rf $(OBJECTS_DIR) $(NAME) ./docs/upload/*.txt

re: fclean all

git :
	git add .
	git commit -m "auto commit"
	git push

.PHONY: all clean fclean re git

