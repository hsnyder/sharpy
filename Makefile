# Makefile requires gmake on BSD

NPY_INC_DIR=$(shell python -c 'import numpy; print(numpy.get_include());')
PY_INC_DIR=$(shell python -c 'import sysconfig; print(sysconfig.get_paths()["include"])') 

SRCFILES=$(shell find src -name '*.c')
HDRFILES=$(shell find src -name '*.h')

Sharpy.so: $(SRCFILES) $(HDRFILES)
	cc -Wall -Wextra -Wswitch-enum -pedantic -Werror -I$(NPY_INC_DIR) -I$(PY_INC_DIR) -Isrc -fPIC -O2 -shared $(SRCFILES) -o $(@)

	

