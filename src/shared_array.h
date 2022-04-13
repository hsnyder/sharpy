/* 
 * This file is part of SharedArray.
 * Copyright (C) 2014-2017 Mathieu Mirmont <mat@parad0x.org>
 * 
 * SharedArray is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * SharedArray is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SharedArray.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SHARED_ARRAY_H__
#define __SHARED_ARRAY_H__

#define NPY_NO_DEPRECATED_API	NPY_1_8_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL	SHARED_ARRAY_ARRAY_API
#define NO_IMPORT_ARRAY

#include <Python.h>
#include <structseq.h>
#include <numpy/arrayobject.h>
#include <stdint.h>
#include <limits.h>

/* Magic header */
#define SHARED_ARRAY_MAGIC 0x5f3759df 

#define SHARED_ARRAY_MAX_DIMS 7
extern const int ACTUAL_MAXDIMS;

/* Array descrdata */
struct array_descr {
	int32_t magic;
	int32_t typenum;
	int64_t shape[SHARED_ARRAY_MAX_DIMS];
	int64_t stride[SHARED_ARRAY_MAX_DIMS];
	int32_t pad[2];
};

_Static_assert(sizeof(struct array_descr) == 128, "Unsupported platform: array descriptor size is not 128 bytes.");

static inline int 
array_descr_ndims(struct array_descr * ad)
{
	unsigned i = 0;
	for(i = 0; i < sizeof(ad->shape)/sizeof(ad->shape[0]) && ad->shape[i] != 0; i++);
	return i & INT_MAX;
}

static inline int
supported_type(enum NPY_TYPES t)
{
	switch (t) {
	case NPY_BOOL:
	case NPY_BYTE:
	case NPY_SHORT:
	case NPY_INT:
	case NPY_LONG:
	case NPY_LONGLONG:
	case NPY_UBYTE:
	case NPY_USHORT:
	case NPY_UINT:
	case NPY_ULONG:
	case NPY_ULONGLONG:
	case NPY_HALF:
	case NPY_FLOAT:
	case NPY_DOUBLE:
	case NPY_LONGDOUBLE:
	case NPY_CFLOAT:
	case NPY_CDOUBLE:
	case NPY_CLONGDOUBLE:
		return 1;
	case NPY_DATETIME:
	case NPY_TIMEDELTA:
	case NPY_STRING:
	case NPY_UNICODE:
	case NPY_OBJECT:
	case NPY_VOID:
	case NPY_NTYPES: 
	case NPY_NOTYPE:
	case NPY_USERDEF:
		return 0;
	default: 
		return 0;
	}
}


/* Main functions */
extern PyObject *shared_array_create(PyObject *self, PyObject *args, PyObject *kwds);
extern PyObject *shared_array_attach(PyObject *self, PyObject *args);
extern PyObject *shared_array_delete(PyObject *self, PyObject *args);

/* Support functions */
extern int open_file(const char *name, int flags, mode_t mode);
extern int unlink_file(const char *name);

#endif /* !__SHARED_ARRAY_H__ */
