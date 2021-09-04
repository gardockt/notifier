OUTPUT = notifier
SRC_DIR = src
BUILD_DIR = build

SRCS = $(shell find $(SRC_DIR) -name *.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

FLAGS = -ggdb
LIBFLAGS = `pkg-config --cflags libnotify` `xml2-config --cflags`
LIBS = -lpthread -liniparser `pkg-config --libs libnotify` -lm `curl-config --libs` -ljson-c `xml2-config --libs`



.PHONY: clean install

all: $(OUTPUT)

clean:
	-rm -r $(BUILD_DIR)/* $(OUTPUT)

install: $(OUTPUT)
	install -D -m0755 $(OUTPUT) /usr/bin



$(OUTPUT): $(OBJS)
	$(CC) $(LIBFLAGS) $(FLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	mkdir -p $(dir $@)
	$(CC) $(LIBFLAGS) $(FLAGS) -o $@ -c $< $(LIBS)
