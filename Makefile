NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 #-fsanitize="address,leak" -fno-omit-frame-pointer -O2 -g3

INC			= inc/
SRCS_DIR	= srcs/
    CFILES			= main/main.cpp \
		 	ServerSocket.cpp \
			utils.cpp Parser.cpp Server.cpp Client.cpp \
			main/print_helpers.cpp main/signals.cpp main/cgi_fds.cpp main/event_loop.cpp


ODIR = build

INCLUDES	= -I$(INC)
SRCS		= $(addprefix $(SRCS_DIR), $(CFILES))
OFILES		= $(addprefix $(ODIR)/, $(CFILES:.cpp=.o))

all: $(NAME)


$(ODIR)/%.o: $(SRCS_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo "🛠️  Compiling $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(NAME): $(OFILES)
	@echo "🔗 Linking $(NAME)..."
	$(CXX) $(CXXFLAGS) $(OFILES) -o $(NAME)
	@echo "✅ $(NAME) compiled successfully!"

clean:
	@echo "🧹 Cleaning objects..."
	rm -rf $(ODIR)

fclean: clean
	@echo "🧼 Removing executable..."
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
