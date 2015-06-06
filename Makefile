CC := cc
CFLAGS := -g3
LIBS := -lssl -lcrypto -lstrophe
SOURCES := shove.c
PROJECT := shove

shove:
	$(CC) $(SOURCES) $(LIBS) $(CFLAGS) -o $(PROJECT)

clean:
	rm $(PROJECT)
