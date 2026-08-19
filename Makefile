COMPILER = c++

FLAGS = -std=c++20 -Wextra -Werror -Wall -fsanitize=address,undefined

DEBUG_FLAGS = $(FLAGS) -ggdb3 -O0

SERVER_SRC = server/server.cpp server/client_loop.cpp

GUI_SRC = gui/gui.cpp

CLI_SRC = cli/cli.cpp


SERVER_BIN = bin/server
GUI_BIN = bin/gui
CLI_BIN = bin/cli

.PHONY: all server gui cli clean fclean re \
		all-debug server-debug gui-debug cli-debug

all: server gui cli

server: $(SERVER_SRC)
	@echo "Compiling server binary..."
	$(COMPILER) $(FLAGS) $(SERVER_SRC) -o $(SERVER_BIN)

gui: $(GUI_SRC)
	@echo "Compiling gui client's binary..."
	$(COMPILER) $(FLAGS) $(GUI_SRC) -o $(GUI_BIN)


cli: $(CLI_SRC)
	@echo "Compiling cli client's binary..."
	$(COMPILER) $(FLAGS) $(CLI_SRC) -o $(CLI_BIN)


all-debug: server-debug gui-debug cli-debug

server-debug:
	@echo "Compiling debug server binary..."
	$(COMPILER) $(DEBUG_FLAGS) $(SERVER_SRC) -o $(SERVER_BIN)

gui-debug:
	@echo "Compiling debug gui client's binary"
	$(COMPILER) $(DEBUG_FLAGS) $(GUI_SRC) -o $(GUI_BIN)

cli-debug:
	@echo "Compiling debug cli client's binary..."
	$(COMPILER) $(DEBUG_FLAGS) $(CLI_SRC) -o $(CLI_BIN)


clean:
	rm -rf $(SERVER_BIN) $(GUI_BIN) $(CLI_BIN)

fclean: clean
	rm *.o

re: fclean all
