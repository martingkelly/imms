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
	objcopy -I binary -O default -B i386:x86-64 --rename-section .data=.rodata,alloc,load,readonly,data,contents $< $@

# macros that expand to the object files in the given directories
objects=$(sort $(notdir $(foreach type,c cc,$(call objects_$(type),$1))))
objects_c=$(patsubst %.c,%.o,$(wildcard $(addsuffix /*.c,$1)))
objects_cc=$(patsubst %.cc,%.o,$(wildcard $(addsuffix /*.cc,$1)))

.PHONY: install analyzer_install immsremote_install

install: all plugins_install programs_install

programs_install: $(patsubst %,%_install,$(OPTIONAL))
	${INSTALL} -D immsd immstool $(bindir)
