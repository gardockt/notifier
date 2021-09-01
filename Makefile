SOURCES = Main.o ModuleManager.o Map.o FetchingModule.o DisplayManager.o Libnotify.o Stash.o Twitch.o StringOperations.o Github.o Dirs.o Isod.o Network.o Display.o Rss.o
OUTPUT = notifier
FLAGS = -ggdb
LIBFLAGS = `pkg-config --cflags libnotify` `xml2-config --cflags`
LIBS = -lpthread -liniparser `pkg-config --libs libnotify` -lm `curl-config --libs` -ljson-c `xml2-config --libs`



.PHONY: clean test

all: $(OUTPUT)

clean:
	rm *.o $(OUTPUT)



$(OUTPUT): $(SOURCES)
	$(CC) $(LIBFLAGS) $(FLAGS) -o $@ $^ $(LIBS)

%.o: */*/%.c */*/%.h
	$(CC) $(LIBFLAGS) $(FLAGS) -c $< $(LIBS)
