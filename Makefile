NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 #-fsanitize="address,leak" -fno-omit-frame-pointer -O2 -g3

INC			= inc/
SRCS_DIR	= srcs/
CFILES		= \
			ServerSocket.cpp \
			Server.cpp \
			utils.cpp \
			main/main.cpp \
			main/print_helpers.cpp \
			main/signals.cpp \
			main/cgi_fds.cpp \
			main/event_loop.cpp \
			client/Client.cpp \
			client/client_cgi.cpp \
			client/client_helpers.cpp \
			client/client_parsing.cpp \
			client/client_path.cpp \
			client/client_response.cpp \
			parse/Parser.cpp \
			parse/parser_parsers.cpp \
			parse/parser_states.cpp \
			parse/parser_validators.cpp \



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
