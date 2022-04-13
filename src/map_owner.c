/* 
 * This file is part of SharedArray.
 * Copyright (C) 2014-2020 Mathieu Mirmont <mat@parad0x.org>
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

#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>
#include <Python.h>
#include <structmember.h>
#include "map_owner.h"

/*
 * Destructor
 */
static void do_dealloc(PyMapOwnerObject *self)
{
	/* Unmap the data */
	if (munmap(self->map_addr, self->map_size) < 0)
		PyErr_SetFromErrno(PyExc_RuntimeError);

	/* Free the file name */
	free(self->name);

	/* Free the object */
	Py_TYPE(self)->tp_free((PyObject*) self);
}

/*
 * List of methods
 */
static struct PyMethodDef methods[] = {
	{ "msync", (PyCFunction) map_owner_msync,
	  METH_VARARGS | METH_KEYWORDS,
	  "Synchronise a file with a memory map (msync(2) wrapper)" },

	{ "mlock", (PyCFunction) map_owner_mlock,
	  METH_VARARGS,
	  "Lock the array in memory (mlock(2) wrapper)" },

	{ "munlock", (PyCFunction) map_owner_munlock,
	  METH_VARARGS,
	  "Unlock the array in memory (munlock(2) wrapper)" },

	{ NULL, NULL, 0, NULL }
};

/*
 * List of members
 */
static struct PyMemberDef members[] = {
	{ "name", T_STRING, offsetof (PyMapOwnerObject, name), 0,
	  "Name of the file used to back this array" },

	{ "addr", T_PYSSIZET, offsetof (PyMapOwnerObject, map_addr), 0,
	  "Base address of the shared array in memory" },

	{ "size", T_PYSSIZET, offsetof (PyMapOwnerObject, map_size), 0,
	  "Size of the shared array in memory" },

	{ NULL, 0, 0, 0, NULL }
};

/*
 * MapOwner type definition
 */
PyTypeObject PyMapOwner_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name           = "shared_array.map_owner",
	.tp_basicsize      = sizeof (PyMapOwnerObject),
	.tp_dealloc        = (destructor) do_dealloc,
	.tp_flags          = Py_TPFLAGS_DEFAULT,
	.tp_methods        = methods,
	.tp_members        = members,
};
