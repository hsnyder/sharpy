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

#include <sys/mman.h>
#include <Python.h>
#include "map_owner.h"

PyObject *map_owner_mlock(PyMapOwnerObject *self, PyObject *args)
{
	(void) args;

	/* Call mlock(2) */
	if (mlock(self->map_addr, self->map_size) < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}
