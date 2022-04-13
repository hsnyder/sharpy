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

#define NPY_NO_DEPRECATED_API	NPY_1_8_API_VERSION
#define PY_ARRAY_UNIQUE_SYMBOL	SHARED_ARRAY_ARRAY_API
#define NO_IMPORT_ARRAY

#include <Python.h>
#include <numpy/arrayobject.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "shared_array.h"
#include "map_owner.h"


static inline long 
align64_roundup(long elsz, long val) 
{
	const long els64 = 64/elsz;
	if (val % els64 == 0) return val;
	return ((val/els64)+1) *els64;
}

/*
 * Create a numpy array in shared memory
 */
static PyObject *do_create(const char *name, int ndims, npy_intp *dims, PyArray_Descr *dtype, int aligned)
{
	struct array_descr *descr;
	size_t size;
	size_t map_size;
	unsigned char *map_addr;
	int fd;
	struct stat file_info;
	PyObject *array;
	PyMapOwnerObject *map_owner;

	/* Check the number of dimensions */
	if (ndims > ACTUAL_MAXDIMS) {
		PyErr_Format(PyExc_ValueError,
			     "number of dimensions must be within [0, %d]",
			     ACTUAL_MAXDIMS);
		return NULL;
	}


	/* Check that the type is supported */
	if (!supported_type(dtype->type_num)) {
		PyErr_Format(PyExc_ValueError,
			     "Unsupported data type %d",
			     dtype->type_num);
		return NULL;
	}

	/* Calculate the memory size of the array */
	int64_t aligned_dims[SHARED_ARRAY_MAX_DIMS] = {};
	for (int i = 0; i < ndims; i++)
		aligned_dims[i] = aligned ? align64_roundup(dtype->elsize, dims[i]) : dims[i]; 
	

	size = dtype->elsize;
	for (int i = 0; i < ndims; i++)
		size *= (aligned && i>0) ? aligned_dims[i] : dims[i];


	/* Calculate the size of the mmap'd area */
	map_size = size + sizeof (*descr);

	/* Create the file */
	if ((fd = open_file(name, O_RDWR | O_CREAT | O_EXCL, 0666)) < 0)
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);

	/* Grow the file */
	if (ftruncate(fd, map_size) < 0) {
		close(fd);
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
	}

	/* Find the actual file size after growing (on some systems it rounds
	 * up to 4K) */
	if (fstat(fd, &file_info) < 0) {
		close(fd);
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
	}
	map_size = file_info.st_size;

	/* Map it */
	map_addr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (map_addr == MAP_FAILED)
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);

	/* Append the descr-data to the array in memory */
	descr = (struct array_descr *) map_addr;
	*descr = (struct array_descr){
		.magic   = SHARED_ARRAY_MAGIC, 
		.typenum = dtype->type_num
	};

	for (int i = 0; i < ndims; i++)
		descr->shape[i] = dims[i];

	for (int i = 0; i < ndims; i++) {
		descr->stride[i] = 1;
		for (int j = i + 1; j < ndims; j++) 
			descr->stride[i] *=  aligned ? aligned_dims[j] : descr->shape[j];
	}

	int64_t strides_bytes[SHARED_ARRAY_MAX_DIMS] = {};
	for (int i = 0; i < array_descr_ndims(descr); i++) 
		strides_bytes[i] = descr->stride[i] * dtype->elsize;


	/* Hand over the memory map to a MapOwner instance */
	map_owner = PyObject_MALLOC(sizeof (*map_owner));
	PyObject_INIT((PyObject *) map_owner, &PyMapOwner_Type);
	map_owner->map_addr = map_addr;
	map_owner->map_size = map_size;
	map_owner->name = strdup(name);

	/* Create the array object */
	array = PyArray_New(&PyArray_Type, array_descr_ndims(descr), descr->shape,
	                    descr->typenum, strides_bytes, map_addr + sizeof(struct array_descr), 0,
	                    NPY_ARRAY_BEHAVED, NULL);

	/* Attach MapOwner to the array */
	PyArray_SetBaseObject((PyArrayObject *) array, (PyObject *) map_owner);
	return array;
}

/*
 * Method: SharedArray.create()
 */
PyObject *shared_array_create(PyObject *self, PyObject *args, PyObject *kwds)
{
	(void) self;

	static char *kwlist[] = { "name", "shape", "dtype", "aligned", NULL };
	const char *name;
	PyArray_Dims shape = { NULL, 0 };
	PyArray_Descr *dtype = NULL;
	PyObject *ret = NULL;
	int aligned = 0;

	/* Parse the arguments */
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "sO&|O&p", kwlist,
					 &name,
					 PyArray_IntpConverter, &shape,
					 PyArray_DescrConverter, &dtype, &aligned))
		goto out;

	/* Check the type */
	if (!dtype)
		dtype = PyArray_DescrFromType(NPY_DEFAULT_TYPE);

	/* Now do the real thing */
	ret = do_create(name, shape.len, shape.ptr, dtype, aligned);

out:	/* Clean-up on exit */
	if (shape.ptr)
		PyDimMem_FREE(shape.ptr);

	return ret;
}
