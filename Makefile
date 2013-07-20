PROJECT		= struct

#
# Flags
#

BINDIR 		= bin
OBJDIR 		= obj
VPATH		= src

C_FILES		= main struct
OBJS		= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(C_FILES)))

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
