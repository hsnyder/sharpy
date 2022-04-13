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

/*
 * Attach a numpy array from shared memory
 */
static PyObject *do_attach(const char *name)
{
	struct array_meta *meta;
	int fd;
	struct stat file_info;
	size_t map_size;
	unsigned char *map_addr;
	PyObject *array;
	PyMapOwnerObject *map_owner;

	/* Open the file */
	if ((fd = open_file(name, O_RDWR, 0)) < 0)
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);

	/* Find the file size */
	if (fstat(fd, &file_info) < 0) {
		close(fd);
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
	}

	/* Ignore short files */
	if (file_info.st_size < (off_t) sizeof (*meta)) {
		close(fd);
		PyErr_SetString(PyExc_IOError, "No SharedArray at this address");
		return NULL;
	}
	map_size = file_info.st_size;

	/* Map the array data */
	map_addr = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (map_addr == MAP_FAILED)
		return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);

	/* Check the meta data */
	meta = (struct array_meta *) (map_addr + (map_size - sizeof (*meta)));
	if (strncmp(meta->magic, SHARED_ARRAY_MAGIC, sizeof (meta->magic))) {
		munmap(map_addr, map_size);
		PyErr_SetString(PyExc_IOError, "No SharedArray at this address");
		return NULL;
	}

	/* Check the number of dimensions */
	if (meta->ndims > NPY_MAXDIMS) {
		munmap(map_addr, map_size);
		PyErr_Format(PyExc_ValueError,
			     "number of dimensions must be within [0, %d]",
			     NPY_MAXDIMS);
		return NULL;
	}

	/* Hand over the memory map to a MapOwner instance */
	map_owner = PyObject_MALLOC(sizeof (*map_owner));
	PyObject_INIT((PyObject *) map_owner, &PyMapOwner_Type);
	map_owner->map_addr = map_addr;
	map_owner->map_size = map_size;
	map_owner->name = strdup(name);

	/* Create the array object */
	array = PyArray_New(&PyArray_Type, meta->ndims, meta->dims,
	                    meta->typenum, NULL, map_addr, meta->itemsize,
	                    NPY_ARRAY_CARRAY, NULL);

	/* Attach MapOwner to the array */
	PyArray_SetBaseObject((PyArrayObject *) array, (PyObject *) map_owner);
	return array;
}

/*
 * Method: SharedArray.attach()
 */
PyObject *shared_array_attach(PyObject *self, PyObject *args)
{
	(void) self;

	const char *name;

	/* Parse the arguments */
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	/* Now do the real thing */
	return do_attach(name);
}
