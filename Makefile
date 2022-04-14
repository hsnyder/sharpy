# Makefile requires gmake on BSD

NPY_INC_DIR=$(shell python -c 'import numpy; print(numpy.get_include());')
PY_INC_DIR=$(shell python -c 'import sysconfig; print(sysconfig.get_paths()["include"])') 

SRCFILES=$(shell find src -name '*.c')
HDRFILES=$(shell find src -name '*.h')
EXTRAFLAGS=

# Wanted to add: -Wswitch-enum -Werror , but some versions of numpy headers break.

Sharpy.so: $(SRCFILES) $(HDRFILES)
	cc -Wall -Wextra -I$(NPY_INC_DIR) -I$(PY_INC_DIR) -Isrc -fPIC -O2 -shared $(EXTRAFLAGS) $(SRCFILES) -o $(@)

	

