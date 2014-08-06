-include rules.mk
export acism ?= .

#---------------- PRIVATE VARS:
# acism.pgms: test programs requiring no args

acism.c          = $(patsubst %,$(acism)/%, acism.c acism_create.c acism_dump.c acism_file.c)
acism.pgms       = $(acism)/acism_x $(acism)/acism_mmap_x
ACISM_SIZE       ?= 4   # size of transition table elements.

#---------------- PUBLIC VARS (see rules.mk)
acism.lib       = $(acism)/libacism.a
acism.include   = $(acism)/acism.h
all             += acism
clean           += $(acism)/*.tmp

#---------------- PUBLIC TARGETS (see rules.mk):
all             : $(acism.lib)
test            : $(acism)/acism_t.pass

# Activate default actions for (clean,install):
install .PHONY  : acism.install

#---------------- PRIVATE RULES:
$(acism.pgms)   : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)
$(acism.lib)	: $(acism.c:c=o)

# ACISM by default defines TRAN as a 4-byte int. This has about a 10MB pattern string limit.
#   An 8-byte TRAN has no practical limit. It is also faster on a 64bit processor,
#   since its fields are const size.

$(acism)/acism_t.pass : $(acism.pgms) $(acism)/words

$(acism.pgms)   : $(acism.lib) $(acism)/msutil.o $(acism)/tap.o
$(acism.pgms)   : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)

$(acism.c:c=s)  : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)
$(acism.c:c=i)  : CPPFLAGS += -DACISM_SIZE=$(ACISM_SIZE)

-include $(acism)/*.d
# vim: set nowrap :
