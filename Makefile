CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++17
SRC = src/main.cpp \
 src/common/Logger.cpp \
 src/server/Server.cpp \
 src/server/ServerPool.cpp \
 src/server/ClientConnection.cpp \
 src/parser/http/HttpParser.cpp \
 src/server/response/HttpResponse.cpp \
 src/server/requestHandler/RequestHandler.cpp \
 src/server/requestHandler/PostRequest.cpp \
 src/server/requestHandler/GetRequest.cpp \
 src/server/requestHandler/DeleteRequest.cpp \
 src/server/requestHandler/PutRequest.cpp \
 src/server/requestHandler/responses.cpp \
 src/server/requestHandler/RequestHandlerUtils.cpp \
 src/server/requestHandler/CGIRequest.cpp


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