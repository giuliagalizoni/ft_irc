CXX      = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iincludes
NAME     = ircserv

SRC_DIR  = src
OBJ_DIR  = obj
SRCS     = main.cpp Server.cpp User.cpp

SRC_FILES = $(addprefix $(SRC_DIR)/, $(SRCS))
OBJS     = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))

all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
