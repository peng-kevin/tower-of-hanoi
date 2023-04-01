BIN := tower-of-hanoi
SRCS := tower-of-hanoi.c

CFLAGS := -Wall -Wextra -Werror -pedantic-errors

# make D=1 to compile with debug flags
# make G=1 to compile with debug flags and optimizations	
ifeq ($(D), 1)
CFLAGS +=  -g -O0
else ifeq ($(G), 1)
CFLAGS += -g -O3 -march=native
else
CFLAGS +=  -O3 -march=native
endif

# make V=1 to compile in verbose mode
ifneq ($(V), 1)
Q = @
endif

.PHONY: all
all: $(BIN)

tower-of-hanoi: $(SRCS)
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	@echo "clean"
	$(Q)rm -f $(BIN)
