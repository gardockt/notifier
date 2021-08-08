LIBS = Main.o ModuleManager.o Map.o FetchingModule.o HelloWorld.o DisplayManager.o Dunst.o Stash.o Twitch.o StringOperations.o Github.o
OUTPUT = notifier
FLAGS = -lpthread -liniparser `pkg-config --cflags --libs libnotify` -lm `curl-config --libs` -ljson-c -ggdb



.PHONY: clean test

all: $(OUTPUT)

clean:
	rm *.o $(OUTPUT)



$(OUTPUT): $(LIBS)
	$(CC) $(FLAGS) -o $@ $^

%.o: */*/%.c */*/%.h
	$(CC) $(FLAGS) -c $<
