CC = gcc
CFLAGS = -O2 -finline-functions -g
CFLAGS += -Wall -Winline
LDFLAGS += -g
AR = ar
LIBS = -lm -lGL -lGLU -lglut

#
# For cygwin, uncomment the next one
#
# LIBS = -lm -lopengl32 -lglu32 -lglut32

#
# For debugging, uncomment the next one
#
# CFLAGS += -O0 -DDEBUG -g3 -gdwarf-2

PROGRAMS = bezier

LIB_H = vector2.h curve.h util.h
LIB_OBJS = curve.o
LIB_FILE = libcurve.a

#
# Pretty print
#
V	      = @
Q	      = $(V:1=)
QUIET_CC      = $(Q:@=@echo    '     CC       '$@;)
QUIET_AR      = $(Q:@=@echo    '     AR       '$@;)
QUIET_GEN     = $(Q:@=@echo    '     GEN      '$@;)
QUIET_LINK    = $(Q:@=@echo    '     LINK     '$@;)

all: $(PROGRAMS)

bezier: main.o  $(LIB_FILE)
	$(QUIET_LINK)$(CC) $(LDFLAGS) -o $@ $< $(LIB_FILE) $(LIBS)

curve.o: $(LIB_H)
main.o: $(LIB_H)

$(LIB_FILE): $(LIB_OBJS)
	$(QUIET_AR)$(AR) rcs $@ $(LIB_OBJS)

.c.o:
	$(QUIET_CC)$(CC) -o $@ -c $(CFLAGS) $<

clean:
	rm -f *.[oa] *.so $(PROGRAMS) $(LIB_FILE)
