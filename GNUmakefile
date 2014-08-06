-include rules.mk
export acism ?= .

#---------------- PRIVATE VARS:
acism.c         = $(patsubst %,$(acism)/%, acism.c acism_create.c acism_dump.c acism_file.c)
acism.lib       = $(acism)/libacism.a
acism.pgms      = $(acism)/acism_x $(acism)/acism_mmap_x

# size in bytes of a transition table element.
ACISM_SIZE      ?= 4   

#---------------- PUBLIC VARS (see rules.mk)
all             += acism
clean           += $(acism)/*.tmp

install.lib     += $(acism.lib)
install.include += $(acism)/acism.h

#---------------- PUBLIC TARGETS (see rules.mk):
all             : $(acism.lib)
test            : $(acism)/acism_t.pass

#---------------- PRIVATE RULES:
$(acism.lib)	: $(acism.c:c=o)

$(acism)/acism_t.pass : $(acism.pgms) $(acism)/words

$(acism.pgms)   : $(acism.lib) $(acism)/msutil.o $(acism)/tap.o
$(acism.pgms)   : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)

$(acism.c:c=s)  : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)
$(acism.c:c=i)  : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)

-include $(acism)/*.d
# vim: set nowrap :
