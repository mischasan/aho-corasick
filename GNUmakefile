-include rules.mk
export acism ?= .

#---------------- PRIVATE VARS:
acism.c         = $(patsubst %,$(acism)/%, acism.c acism_create.c acism_dump.c acism_file.c)
acism.x         = $(acism)/acism_x $(acism)/acism_mmap_x

#---------------- PUBLIC VARS (see rules.mk): all clean install.* source tags
all             += acism
clean           += $(acism)/*.tmp
install.lib     += $(acism)/libacism.a
install.include += $(acism)/acism.h

# This is easy but affects multi-project builds:
CPPFLAGS        += -DACISM_SIZE=4

#---------------- PUBLIC TARGETS (see rules.mk):
all             : $(acism.lib)
test            : $(acism)/acism_t.pass

#---------------- PRIVATE RULES:
$(acism)/libacism.a	    : $(acism.c:c=o)

$(acism)/acism_t.pass   : $(acism.x) $(acism)/words

$(acism.x)      : $(acism)/libacism.a $(acism)/msutil.o $(acism)/tap.o

-include $(acism)/*.d
# vim: set nowrap :
