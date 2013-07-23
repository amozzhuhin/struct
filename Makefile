PROJECT		= struct-tests

#
# Flags
#

BINDIR 		= bin
OBJDIR 		= obj
VPATH		= src

C_FILES		= tests struct
OBJS		= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(C_FILES)))

CFLAGS		= -Wall -Wextra -Wno-unused-parameter -Wformat-y2k -Winit-self \
			  -Wstrict-prototypes -Winline -Wnested-externs -Wbad-function-cast -Wshadow

#
# Targets
#

.PHONY: clean all prepare

prepare:
	mkdir -p $(BINDIR)
	mkdir -p $(OBJDIR)

clean:
	rm $(BINDIR)/$(PROJECT)
	rm -r $(BINDIR)
	rm $(OBJS)
	rm -r $(OBJDIR)

all: prepare $(OBJS)
	$(CC) $(OBJS) -o $(BINDIR)/$(PROJECT) $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -g -c $^ -o $@
