# Copyright (C) 2009-2014 Mischa Sandberg <mischasan@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License Version 2 as
# published by the Free Software Foundation.  You may not use, modify or
# distribute this program under any other version of the GNU Lesser General
# Public License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#------------------------------------------------------------------------

# RULES.MK input vars:
#   $(BLD)      : |cover|debug|profile  -- default is optimized (release) build.
#   $(all)      : submodules; "make clean" cleans up various side-effect files here.
#   $(clean)    : extra files to clean
#   $(install.{bin,etc,include,ini,sbin}) : files for "make install"

ifndef RULES_MK
RULES_MK:=1 # Allow repeated "-include".

#---- Environment:
export LD_LIBRARY_PATH
export PS4      = \# # Prefix for "sh -x" output.
export SHELL    = /bin/bash

# Import from PREFIX, export to DESTDIR.
PREFIX          ?= /usr/local
DESTDIR         ?= $(PREFIX)
OS              := $(shell uname -s)

# HACK CentOS 5 comes with gcc 4.1, gcc 4.4 requires a special command
#CC              = /usr/bin/gcc44

#--- *.$(BLD):
CFLAGS.         = -O2

CFLAGS.cover    = --coverage -DNDEBUG
LDFLAGS.cover   = --coverage

CFLAGS.debug    = -O0 -Wno-uninitialized
CPPFLAGS.debug  = -UNDEBUG

CFLAGS.profile  = -pg -DNDEBUG
LDFLAGS.profile = -pg

# PROFILE tests get stats on syscalls in their .pass files.
exec.profile	= strace -cf

#--- *.$(OS):
CFLAGS.Darwin   = 
LDLIBS.FreeBSD  = -lm
LDLIBS.Linux    = -lm   # floor()

# Before gcc 4.5, -Wno-unused-result was unknown and causes an error.
Wno-unused-result := $(shell $(CC) -dumpversion | awk '$$0 >= 4.5 {print "-Wno-unused-result"}')

CFLAGS          += -ggdb -MMD -fdiagnostics-show-option -fstack-protector --param ssp-buffer-size=4 -fno-strict-aliasing
CFLAGS          += -Wall -Werror -Wextra -Wcast-align -Wcast-qual -Wformat=2 -Wformat-security -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wno-unknown-pragmas -Wunused -Wwrite-strings
CFLAGS          += -Wno-attributes -Wno-cast-qual -Wno-unknown-pragmas $(Wno-unused-result)
CFLAGS          += $(CFLAGS.$(BLD)) $(CFLAGS.$(OS))
CXXFLAGS        += $(filter-out -Wmissing-prototypes -Wnested-externs -Wstrict-prototypes, $(CFLAGS))

# -D_FORTIFY_SOURCE=2 on some platforms rejects any libc call whose return value is ignored.
#   For some calls (system, write) this makes sense. For others (vasprintf), WTF?

CPPFLAGS        += -I$(PREFIX)/include -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE $(CPPFLAGS.$(BLD)) $(CPPFLAGS.$(OS))
LDFLAGS         += -L$(PREFIX)/lib $(LDFLAGS.$(BLD)) $(LDFLAGS.$(OS))
LDLIBS          += $(LDLIBS.$(OS))

#---------------- Explicitly CANCEL EVIL BUILTIN RULES:
%               : %.c 
%               : %.cpp
%.c             : %.l
%.c             : %.y
%.r             : %.l
#----------------
.PHONY          : all clean cover debug gccdefs install profile source tags test
.DEFAULT_GOAL   := all

# $(all) contains all subproject names. It can be used in ACTIONS but not RULES,
#   since it accumulates across "include */GNUmakefile"'s.

# All $(BLD) types use the same pathnames for binaries.
# To switch from release to debug, first "make clean".
# To extract and save exports, "make install DESTDIR=rel".

all             :;@echo "$@ done for BLD='$(BLD)'"
junkfiles       = gmon.out,tags,*.fail,*.gcda,*.gcno,*.gcov,*.prof
clean           :;@rm -rf $(shell $(MAKE) -nps all test cover profile | sed -n '/^# I/,$${/^[^\#\[%.][^ %]*: /s/:.*//p;}') \
                          $(addsuffix /{$(junkfiles)}, $(all))  $(clean)  $(filter %.d, $(MAKEFILE_LIST))

cover           : BLD := cover
cover           : test    ; gcov -bcp $($@) | covsum    # DOES NOT WORK

debug           : BLD := debug
debug           : all

#---- Macro functions:
# Expand: translate every occurrence of "${var}" in a file to its env value (or ""):
Expand          = perl -pe 's/ (?<!\\) \$${ ([A-Z_][A-Z_0-9]*) } / $$ENV{$$1} || ""/geix'

Install         = if [ "$(strip $1)" ]; then mkdir -p $2; pax -rwpe -s:.*/:: $1 $2; fi
install         : $(addprefix install., bin etc include ini lib man1 man3 sbin)
install.man%    :;$(call Install, $($@), $(DESTDIR)/man/$(@:install.))
install.%       :;$(call Install, $($@), $(DESTDIR)/$(@:install.))

profile         : BLD := profile
%.profile       : test    ;@for x in $($*.test:.pass=); do gprof -b $$x >$$x.prof; done

# GMAKE trims leading "./" from $*.; $(*D)/$(*F) restores it.
%.pass          : %         ; rm -f $@; $(exec.$(BLD)) $(*D)/$(*F) >& $*.fail && mv -f $*.fail $@

# To build a .so, "make clean" first, to ensure all .o files compiled with -fPIC
%.so            : CFLAGS := -fPIC $(filter-out $(CFLAGS.cover) $(CFLAGS.profile), $(CFLAGS))
%.so            : %.o       ; $(CC) $(LDFLAGS) -o $@ -shared $< $(LDLIBS)
%.so            : %.a       ; $(CC) $(CFLAGS)  -o $@ -shared -Wl,-whole-archive $< -Wl,-no-whole-archive $(LDLIBS)
%.a             :           ; [ "$^" ] && ar crs $@ $(filter %.o,$^)
%.yy.c          : %.l       ; flex -o $@ $<
%.tab.c 	    : %.y       ; bison $<
%/..            :           ;@mkdir -p $(@D)
%               : %.gz      ; gunzip -c $^ >$@

# Ensure that intermediate files (e.g. the foo.o caused by "foo : foo.c")
#  are not auto-deleted --- causing a re-compile every second "make".
.SECONDARY  	: 

#---------------- TOOLS:
# NOTE: "source" MUST be set with "=", not ":=", else MAKE recurses infinitely.
source          = $(filter-out %.d, $(shell $(MAKE) -nps all test cover profile | sed -n '/^. Not a target/{n;/^[^.*][^ ]*:/s/:.*//p;}'))

# gccdefs : all gcc internal #defines.
gccdefs         :;@$(CC) $(CPPFLAGS) -E -dM - </dev/null | cut -c8- | sort

tags            :; ctags $(filter %.c %.cpp %.h, $(source))

# sh : invoke a shell within the makefile's env:
sh   		    :; PS1='$(PS1) [make] ' $(SHELL)

# "make SomeVar." prints $(SomeVar)
%.              :;@echo '$($*)'

# %.I lists all (recursive) #included files; e.g.: "make /usr/include/errno.h.I"
%.I             : %.c       ;@ls -1 2>&- `$(CC)  $(CPPFLAGS) -M $<` ||:
%.I             : %.cpp     ;@ls -1 2>&- `$(CXX) $(CPPFLAGS) -M $<` ||:
%.i             : %.c       ; $(COMPILE.c)   -E -o $@ $<
%.i             : %.cpp     ; $(COMPILE.cpp) -E -o $@ $<
%.s             : %.c       ; $(COMPILE.c)   -S -o $@ $< && deas $@
%.s             : %.cpp     ; $(COMPILE.cpp) -S -o $@ $< && deas $@

endif
# vim: set nowrap :
