NAME := webserv
CC = g++
INC	= -I ./srcs/Config/ -I ./srcs/SocketInterface/ -I ./srcs/CoreHandler/ \
		-I ./srcs/SocketInterface/RequestParser/ -I ./srcs/SocketInterface/CgiParser -I ./srcs/CoreHandler/StaticFileReader/ \
		-I ./srcs/CoreHandler/DataProcessor -I ./srcs/Cgi/ -I ./srcs/utils/
CFLAGS = -Wall -Wextra -Werror -std=c++98 $(INC)
LDFLAGS =
SOURCES = srcs/main.cpp srcs/Config/Config.cpp srcs/SocketInterface/SocketInterface.cpp \
			srcs/SocketInterface/RequestParser/RequestParser.cpp \
			srcs/SocketInterface/CgiParser/CgiParser.cpp \
			srcs/CoreHandler/CoreHandler.cpp \
			srcs/CoreHandler/ParseRequestUrl.cpp \
			srcs/Config/ConfigError.cpp srcs/Config/ConfigParser.cpp \
			srcs/Config/CGIContext.cpp \
			srcs/Config/LocationContext.cpp srcs/Config/ServerContext.cpp \
			srcs/CoreHandler/StaticFileReader/StaticFileReader.cpp \
			srcs/CoreHandler/DataProcessor/DataProcessor.cpp \
			srcs/Cgi/Cgi.cpp \
			srcs/utils/defaultError.cpp srcs/utils/time.cpp srcs/utils/cgi.cpp  srcs/utils/split.cpp 
OBJECTS_DIR = objs
OBJECTS = $(addprefix $(OBJECTS_DIR)/, $(SOURCES:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJECTS_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJECTS_DIR)

fclean:
	rm -rf $(OBJECTS_DIR) $(NAME)

re: fclean all

git :
	git add -A
	git commit -m "auto commit"
	git push

bonus: all

.PHONY: all clean fclean re git

