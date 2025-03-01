NAME	= ircserv	
CC		= c++
FLAGS	= -Wall -Wextra -Werror -std=c++98
RM		= rm -rf
INCLUDE = -I ./include

OBJDIR = obj
SRC 	  = main.cpp network/Client.cpp network/Server.cpp network/Channel.cpp commands/kick.cpp \
			commands/topic.cpp
OBJ_FILES = $(addprefix $(OBJDIR)/, $(SRC:.cpp=.o))

$(OBJDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(INCLUDE) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJ_FILES)
	$(CC) $(FLAGS) $(INCLUDE) $(OBJ_FILES) -o $(NAME)

clean:
	$(RM) $(OBJ_FILES)
	$(RM) $(OBJDIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all fclean clean re