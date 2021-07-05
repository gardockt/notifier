LIBS = Main.o ModuleManager.o Map.o FetchingModule.o HelloWorld.o DisplayManager.o Dunst.o
TARGET = linux
OUTPUT = notifier
FLAGS = -lpthread -liniparser `pkg-config --cflags --libs libnotify` -ggdb



.PHONY: clean test

all: $(OUTPUT)

clean:
	rm *.o $(OUTPUT)



$(OUTPUT): $(LIBS)
	$(CC) $(FLAGS) -o $@ $^

%.o: */*/%.c */*/%.h
	$(CC) $(FLAGS) -c $<
