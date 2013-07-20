PROJECT		= struct

#
# Flags
#

OBJDIR 		= obj
VPATH		= src

C_FILES		= main struct
OBJS		= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(C_FILES)))

#
# Targets
#

.PHONY: clean all prepare

prepare:
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) 
	rm -f $(PROJECT)

all: prepare $(OBJS)
	$(CC) $(OBJS) -o $(PROJECT) $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -g -c $^ -o $@
