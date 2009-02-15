#
#  IMMS: Intelligent Multimedia Management System
#  Copyright (C) 2001-2009 Michael Grigoriev
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
-include ../clients/*/rules.mk

# usage compile(compiler, source, output, flags) 
DEPFILE = $(if $(filter %.o,$3),$(dir $3).$(notdir $(3:.o=.d)),/dev/null)
define compile
        @$1 $4 $2 -M -E > $(DEPFILE)
        $1 $4 -c $2 -o $3
endef

# usage link(objects, output, flags) 
link = $(CXX) $(filter-out %.a,$1) $(filter %.a,$1) $3 -o $2

%.o: %.cc; $(call compile, $(CXX), $<, $@, $($*-CXXFLAGS) $(CXXFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))
%.o: %.c; $(call compile, $(CC), $<, $@, $($*-CFLAGS) $(CFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))
%: %.o; $(call link, $^ $($*-OBJ) $(LIBS), $@, $($*-LIBS) $(LDFLAGS))
%.so:
	$(CXX) $^ $($*-OBJ) $($*-LIBS) $(LIBS) \
	    $(LDFLAGS) \
            -shared -Wl,-z,defs,-soname,$@ -o $@

%-data.o: %
	$(OBJCOPY) -I binary -O $(OBJCOPYTARGET) -B $(OBJCOPYARCH) --rename-section .data=.rodata,alloc,load,readonly,data,contents $< $@

# macros that expand to the object files in the given directories
objects=$(sort $(notdir $(foreach type,c cc,$(call objects_$(type),$1))))
objects_c=$(patsubst %.c,%.o,$(wildcard $(addsuffix /*.c,$1)))
objects_cc=$(patsubst %.cc,%.o,$(wildcard $(addsuffix /*.cc,$1)))

.PHONY: install analyzer_install immsremote_install

install: all plugins_install programs_install

programs_install: $(patsubst %,%_install,$(OPTIONAL))
	${INSTALL} -D immsd immstool $(bindir)
