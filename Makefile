##
## Ion xinerama module Makefile
##
##

# System specific configuration is in system.mk
TOPDIR=../ion-3/
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) $(X11_INCLUDES) -I$(TOPDIR)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)
LDFLAGS += -Wl,--no-as-needed,-lXext,-lXinerama

SOURCES=mod_xinerama.c

MAKE_EXPORTS=xinerama_module
LIBS = $(X11_LIBS) -lXinerama
MODULE=mod_xinerama

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done

######################################

.PHONY: tarball
tarball: $(SOURCES) $(ETC) $(DOCS) Makefile
	sh -c 'BASENAME=ion-devel-$(MODULE)-`date -r \`ls -t $+ | head -n 1\` +%Y%m%d`; \
		mkdir $$BASENAME; \
		cp $+ $$BASENAME; \
		tar -cvjf $$BASENAME.tar.bz2 $$BASENAME/*; \
		rm -Rf $$BASENAME'

######################################

.PHONY: tags
tags:
	exuberant-ctags -R . $(TOPDIR)