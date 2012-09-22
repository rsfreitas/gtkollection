
CC = gcc
INSTALL = install
TARGET = gtkollection

prefix = $(DESTDIR)/opt/gtkollection
DEST_BIN_DIR = $(prefix)/bin
DEST_PLUGIN_DIR = $(prefix)/plugins

APP_NAME = gtkollection
APP_LINK = /usr/bin/gtkollection
LICENSE = ../misc/gpl-2.0.txt

PLUGINS =	\
	../misc/pl_images

GTK_INCLUDES = $(shell pkg-config --cflags-only-I gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs-only-l gtk+-2.0)

INCLUDEDIR = -I.
CFLAGS = -Wall -Wextra -O0 -ggdb $(INCLUDEDIR) $(GTK_INCLUDES)

LIBDIR =
LIBS = -lsqlite3

HEADERS =	\
	gtkollection.h

OBJS =	\
	collections_dialog.o	\
	collections_notebook.o	\
	config.o		\
	common.o		\
	database.o		\
	image_dialog.o		\
	main.o			\
	gtk_gui.o

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $^ $(LIBDIR) $(LIBS) $(GTK_LIBS)

collections_dialog.o: collections_dialog.c $(HEADERS)
collections_notebook.o: collections_notebook.c $(HEADERS)
config.o: config.c $(HEADERS)
common.o: common.c $(HEADERS)
database.o: database.c $(HEADERS)
image_dialog.o: image_dialog.c $(HEADERS)
main.o: main.c $(HEADERS)
gtk_gui.o: gtk_gui.c $(HEADERS)

clean:
	rm -rf $(OBJS) $(TARGET) *~ ../include/*~

install: $(TARGET)
	$(shell if ! test -d $(DEST_BIN_DIR); then mkdir -p $(DEST_BIN_DIR); fi)
	$(shell if ! test -d $(DEST_PLUGIN_DIR); then mkdir -p $(DEST_PLUGIN_DIR); fi)
	$(INSTALL) -m 755 $(TARGET) $(DEST_BIN_DIR)
	$(INSTALL) -m 755 $(PLUGINS) $(DEST_PLUGIN_DIR)
	$(INSTALL) -m 644 $(LICENSE) $(prefix)
	rm -f $(APP_LINK)
	ln -s $(DEST_BIN_DIR)/$(APP_NAME) $(APP_LINK)

purge: clean $(TARGET)

MANPAGE = ../doc/gtkollection.1
MAN_DIR = /usr/share/man/man1

doc:
	cat $(MANPAGE) | gzip > $(MANPAGE).gz
	fakeroot mv $(MANPAGE).gz $(MAN_DIR)

