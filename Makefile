CCFLAGS := -Wall -Werror
CC_WASM := clang

WASM_EXPORTS := on_mousedown on_keydown on_keyup on_timer globals_init
WASM_EXPORTS_FLAGS := $(foreach f,$(WASM_EXPORTS),-Wl,--export,$(f))

CCFLAGS_WASM := $(CCFLAGS) -O2 --target=wasm32
LDFLAGS_WASM := -nostdlib -Wl,--no-entry $(WASM_EXPORTS_FLAGS) -Wl,--import-undefined, -Wl,--fatal-warnings

WASM_FILES := game.c wasmlib.c gfx.c

.PHONY: all
all: game.js embed

game.wasm: $(WASM_FILES) $(wildcard *.h)
	$(CC_WASM) $(CCFLAGS_WASM) $(WASM_FILES) $(LDFLAGS_WASM) -o $@

game.js: game.wasm embed
	./embed wasmBinary < $< > $@

embed: embed.c
	gcc $(CCFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f embed game.js game.wasm
