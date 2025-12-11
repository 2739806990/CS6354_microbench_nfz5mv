CC      := clang
CFLAGS  := -O2 -Wall -Wextra -std=c11
INC     := -Iinclude

SRC     := $(wildcard src/*.c)
BIN     := $(patsubst src/%.c,bin/%,$(SRC))

all: $(BIN)

# 07_cache_latency.c 单独用 -O0
bin/07_cache_latency: src/07_cache_latency.c src/harness.c
	@mkdir -p bin
	$(CC) -O0 -Wall -Wextra -std=c11 $(INC) $^ -o $@
	@echo ">> Compiled 07_cache_latency with -O0 (required for pointer chasing)"

bin/011_smt_sim: src/011_smt_sim.c src/harness.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $(INC) $^ -o $@

bin/harness: src/harness.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $(INC) -DHARNESS_STANDALONE $^ -o $@

bin/%: src/%.c src/harness.c
	@mkdir -p bin
	$(CC) $(CFLAGS) $(INC) $^ -o $@

run: all
	@chmod +x scripts/run_all.sh
	./scripts/run_all.sh

clean:
	rm -rf bin build