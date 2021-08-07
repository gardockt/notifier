LIBS = Main.o ModuleManager.o Map.o FetchingModule.o HelloWorld.o DisplayManager.o Dunst.o Stash.o
TARGET = linux
OUTPUT = notifier
FLAGS = -lpthread -liniparser `pkg-config --cflags --libs libnotify` -lm -ggdb



.PHONY: clean test

all: $(OUTPUT)

clean:
	rm *.o $(OUTPUT)



$(OUTPUT): $(LIBS)
	$(CC) $(FLAGS) -o $@ $^

%.o: */*/%.c */*/%.h
	$(CC) $(FLAGS) -c $<
