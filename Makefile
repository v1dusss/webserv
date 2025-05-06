CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++17 -g

VPATH = src \
		src/common \
		src/config \
		src/parser \
		src/parser/config \
		src/parser/http \
		src/server \
		src/server/requestHandler \
		src/server/response

SRC = main.cpp \
	Logger.cpp \
	Server.cpp \
	ServerPool.cpp \
	ClientConnection.cpp \
	HttpParser.cpp \
	HttpResponse.cpp \
	RequestHandler.cpp \
	PostRequest.cpp \
	GetRequest.cpp \
	DeleteRequest.cpp \
	PutRequest.cpp \
	RequestHandlerUtils.cpp \
	CGIRequest.cpp \
	ConfigParser.cpp \
	ConfigBlock.cpp

OBJ_DIR = obj
INCLUDE_DIR = src
OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))
OBJ_DIRS = $(dir $(OBJ))

NAME = webserv

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJ)
	@echo "$(GREEN)$(NAME) compiled successfully!                               $(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(eval TOTAL := $(words $(SRC)))
	@$(eval PROGRESS := $(shell echo $$(($(PROGRESS)+1))))
	@$(eval PERCENT := $(shell echo $$(($(PROGRESS)*100/$(TOTAL)))))
	@$(call progress_bar,$(PERCENT))
	@$(CC) $(CFLAGS)   -I$(INCLUDE_DIR) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)$(NAME) object files removed!"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)$(NAME) removed!"

re: fclean all

.PHONY: all clean fclean re

RED     := $(shell tput setaf 1)
GREEN   := $(shell tput setaf 2)
YELLOW  := $(shell tput setaf 3)
BLUE    := $(shell tput setaf 4)
MAGENTA := $(shell tput setaf 5)
CYAN    := $(shell tput setaf 6)
WHITE   := $(shell tput setaf 7)
RESET   := $(shell tput sgr0)

define progress_bar
	@printf "$(CYAN)["; \
	for i in $(shell seq 1 50); do \
		if [ $$i -le $$(($(1)*50/100)) ]; then \
			printf "$(GREEN)█$(RESET)"; \
		else \
			printf "$(WHITE)░$(RESET)"; \
		fi; \
	done; \
	printf "$(CYAN)] %3d%%$(RESET)\r" $(1);
endef