ifeq ($(words $(MAKECMDGOALS)),2)
  ANIMATION := $(word 2, $(MAKECMDGOALS))
endif

ANIMATION_HEADER = animations/$(ANIMATION)_anim.h
ANIMATION_FLAG=ENABLE_RGB_MATRIX_$(shell echo '${ANIMATION}' | tr '[:lower:]' '[:upper:]')
CFLAGS += -DANIMATION_HEADER=\"$(ANIMATION_HEADER)\" -D$(ANIMATION_FLAG)

build:
	clang $(CFLAGS) main.c -o simulator \
		-I/opt/homebrew/Cellar/sdl2/2.32.6/include \
		-L/opt/homebrew/Cellar/sdl2/2.32.6/lib -lSDL2  -O3

run: build
	./simulator
