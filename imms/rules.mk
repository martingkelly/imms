# usage compile(compiler, source, output, flags) 
DEPFILE = $(if $(filter %.o,$3),$(dir $3).$(notdir $(3:.o=.d)),/dev/null)
define compile
        @$1 $4 $2 -M -E > $(DEPFILE)
        $1 $4 -c $2 -o $3
endef

# usage link(objects, output, flags) 
link = $(CXX) $3 $(filter-out %.a,$1) $(filter %.a,$1) -o $2

%.o: %.cc; $(call compile, $(CXX), $<, $@, $($*-CXXFLAGS) $(CXXFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))
%.o: %.c; $(call compile, $(CC), $<, $@, $($*-CFLAGS) $(CFLAGS) $($*-CPPFLAGS) $(CPPFLAGS))
%: %.o; $(call link, $^ $($*-OBJ) $($*-LIBS) $(LIBS), $@, $(LDFLAGS))

# macros that expand to the object files in the given directories
objects=$(sort $(notdir $(foreach type,c cc,$(call objects_$(type),$1))))
objects_c=$(patsubst %.c,%.o,$(wildcard $(addsuffix /*.c,$1)))
objects_cc=$(patsubst %.cc,%.o,$(wildcard $(addsuffix /*.cc,$1)))

.PHONY: install install-user install-system user-message system-message

ifeq ($(shell id -u), 0)
    install: system-message install-system
else
    install: user-message install-user
endif

define installprogs
    ${INSTALL_PROGRAM} analyzer immsremote immstool ${PREFIX}/bin
endef

system-message:
	$(warning Defaulting to installing for all users.)
	$(warning Use 'make install-user' to install for the current user only.)

install-system: all
	${INSTALL_PROGRAM} libimms.so `xmms-config --general-plugin-dir`
	$(call installprogs)

user-message:
	$(warning Defaulting to installing for current user only.)
	$(warning Use 'make install-system' to install for all users.)

install-user: all
	mkdir -p ${HOME}/.xmms/Plugins/General/
	${INSTALL_PROGRAM} libimms.so ${HOME}/.xmms/Plugins/General/
	$(call installprogs)
