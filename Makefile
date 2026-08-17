COMPILER = c++

FLAGS = -Wextra -Werror -Wall -fsanitize=address,undefined

DEBUG_FLAGS = $(FLAGS) -ggdb3 -O0

SERVER_FILES =

GUI_FILES =

CLI_FILES =




server:
	@echo "Compiling server binary..."
	$(COMPILER) $(FLAGS) $(SERVER_FILES)

gui:
	@echo "Compiling gui client's binary..."
	$(COMPILER) $(FLAGS) $(GUI_FILES)


cli:
	@echo "Compiling cli client's binary..."
	$(COMPILER) $(FLAGS) $(CLI_FILES)


all: server gui cli


all-debug:
	@echo "Compiling debug server binary..."
	$(COMPILER) $(DEBUG_FLAGS) $(SERVER_FILES)
	@echo "Compiling debug gui client's binary"
	$(COMPILER) $(DEBUG_FLAGS) $(GUI_FILES)
	@echo "Compiling debug cli client's binary..."
	$(COMPILER) $(DEBUG_FLAGS) $(CLI_FILES)


server-debug:
	@echo "Compiling debug server binary..."
	$(COMPILER) $(DEBUG_FLAGS) $(SERVER_FILES)

gui-debug:
	@echo "Compiling debug gui client's binary"
	$(COMPILER) $(DEBUG_FLAGS) $(GUI_FILES)

cli-debug:
	@echo "Compiling debug cli client's binary..."
	$(COMPILER) $(DEBUG_FLAGS) $(CLI_FILES)
