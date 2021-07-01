LIBS = Main.o ModuleManager.o Map.o FetchingModule.o HelloWorld.o
TARGET = linux
OUTPUT = notifier
FLAGS = -lpthread -ggdb



.PHONY: clean test

all: $(OUTPUT)

clean:
	rm *.o $(OUTPUT)*



$(OUTPUT): $(LIBS)
	$(CC) $(FLAGS) -o $@ $^

%.o: */*/%.c */*/%.h
	$(CC) $(FLAGS) -c $<
