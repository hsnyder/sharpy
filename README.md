(This is a fork of SharedArray by tenzing, which can be found at:
 https://gitlab.com/tenzing/shared-array).
The vast majority of the credit goes to tenzing.


# SharedArray python/numpy extension

This is a simple python extension that lets you share numpy arrays
with other processes on the same computer. It uses either shared files
or POSIX shared memory as data stores and therefore should work on
most operating systems.

## Example

Here's a simple example to give an idea of how it works. This example
does everything from a single python interpreter for the sake of
clarity, but the real point is to share arrays between python
interpreters.

```python
import numpy as np
import SharedArray as sa

# Create an array in shared memory.
a = sa.create("shm://test", 10)

# Attach it as a different array. This can be done from another
# python interpreter as long as it runs on the same computer.
b = sa.attach("shm://test")

# See how they are actually sharing the same memory.
a[0] = 42
print(b[0])

# Destroying a does not affect b.
del a
print(b[0])

# See how "test" is still present in shared memory even though we
# destroyed the array a. This method only works on Linux.
sa.list()

# Now destroy the array "test" from memory.
sa.delete("test")

# The array b is still there, but once you destroy it then the
# data is gone for real.
print(b[0])
```

## Functions

### SharedArray.create(name, shape, dtype=float)

This function creates an array in shared memory and returns a numpy
array that uses the shared memory as data backend.

The shared memory is identified by `name`, which can use the `file://`
prefix to indicate that the data backend will be a file, or `shm://`
to indicate that the data backend shall be a POSIX shared memory
object. For backward compatibility `shm://` is assumed when no prefix
is given. Most operating systems implement strong file caching so
using a file as a data backend won't usually affect performance.

The `shape` and `dtype` arguments are identical to those of the numpy
function `numpy.zeros()`, and the returned array is indeed initialized
to zeros.

The content of the array lives in shared memory and/or in a file and
won't be lost when the numpy array is deleted, nor when the python
interpreter exits. To delete a shared array and reclaim system
resources use the `SharedArray.delete()` function.
 
### SharedArray.attach(name)

This function attaches a previously created array in shared memory
identified by `name`, which can use the `file://` prefix to indicate
that the array is stored as a file, or `shm://` to indicate that the
array is stored as a POSIX shared memory object. For backward
compatibility `shm://` is assumed when no prefix is given.

An array may be simultaneously attached from multiple different
processes (i.e. python interpreters).

The content of the array lives in shared memory and/or in a file and
won't be lost when the numpy array is deleted, nor when the python
interpreter exits. To delete a shared array reclaim system resources
use the `SharedArray.delete()` function.

### SharedArray.delete(name)

This function destroys the previously created array identified by
`name`, which can use the `file://` prefix to indicate that the array
is stored as a file, or `shm://` to indicate that the array is stored
as a POSIX shared memory object. For backward compatibility `shm://`
is assumed when no prefix is given

After calling `delete`, the array will not be attachable anymore, but
existing attachments will remain valid until they are themselves
destroyed. The data is reclaimed by the system when the very last
attachment is deleted.

### SharedArray.list()

This function returns a list of previously created arrays stored as
POSIX SHM objects, along with their name, data type and dimensions.
This function only works on Linux because it directly accesses files
exposed under `/dev/shm`. There doesn't seem to be a portable method
of achieving this.

## Constants

### SharedArray.MS_ASYNC

Flag for the `msync()` method of the base object of the returned numpy
array (see below). Specifies that an update be scheduled, but the call
returns immediately.

### SharedArray.MS_SYNC

Flag for the `msync()` method of the base object of the returned numpy
array (see below). Requests an update and waits for it to complete.

### SharedArray.MS_INVALIDATE

Flag for the `msync()` method of the base object of the returned numpy
array (see below). Asks to invalidate other mappings of the same file
(so that they can be updated with the fresh values just written).

## Base object

SharedArray registers its own python object as the
[base](https://docs.scipy.org/doc/numpy/reference/generated/numpy.ndarray.base.html)
object of the returned numpy array. This base object exposes the
following methods and attributes:

### msync(array, flags)

This method is a wrapper around `msync(2)` and is only useful when
using file-backed arrays (i.e. not POSIX shared memory). msync(2)
flushes the mapped memory region back to the filesystem. The `flags`
are exported as constants in the module definition (see above) and are
a 1:1 map of the `msync(2)` flags, please refer to the manual page of
`msync(2)` for details.

### mlock(array)

This method is a wrapper around `mlock(2)`: lock the memory map into
RAM, preventing that memory from being paged to the swap area.

### munlock(array)

This method is a wrapper around `munlock(2)`: unlock the memory map,
allowing that memory to be paged to the swap area.

### name

This constant string is the name of the array as passed to
`SharedArray.create()` or `SharedArray.attach()`. It may be passed to
`SharedArray.delete()`.

### addr

Base address of the array in memory.

### size

Size of the array in memory.

## Requirements

* Python 2.7 or 3+
* Numpy 1.8+
* Posix shared memory interface

SharedArray uses the posix shm interface (`shm_open` and `shm_unlink`)
and so should work on most POSIX operating systems (Linux, BSD,
etc.). It has been reported to work on macOS, and it is unlikely to
work on Windows.

## Installation

The extension uses the `distutils` python package that should be
familiar to most python users. To test the extension directly from the
source tree, without installing, type:

```sh
python setup.py build_ext --inplace
```

To build and install the extension system-wide, type:

```sh
python setup.py build
sudo python setup.py install
```

The package is also available on PyPI and can be installed using the
pip tool.

## FAQ

### On Linux, I get segfaults when working with very large arrays.

A few people have reported segfaults with very large arrays using
POSIX shared memory. This is not a bug in SharedArray but rather an
indication that the system ran out of POSIX shared memory.

On Linux a `tmpfs` virtual filesystem is used to provide POSIX shared
memory, and by default it is given only about 20% of the total
available memory, depending on the distribution. That amount can be
changed by re-mounting the `tmpfs` filesystem with the `size=100%`
option:

```sh
sudo mount -o remount,size=100% /run/shm
```

Also you can make the change permanent, on next boot, by setting
`SHM_SIZE=100%` in `/etc/defaults/tmpfs` on recent Debian
installations.

### On Linux, I get "Cannot allocate memory" when creating many arrays.

SharedArray uses one memory map per array that is attached (or
created). By default the maximum number of memory maps per process is
set by the Linux kernel to 65530. If you want to create more arrays
than that you need to tune the kernel parameter `vm.max_map_count` and
set it to a higher value.

```sh
/sbin/sysctl vm.max_map_count=655300
```

Note that for the change to be permanent you need to add this line to
`/etc/sysctl.conf`:
```sh
vm.max_map_count=655300
```

### I can't attach old (pre 0.4) arrays anymore.

Since version 0.4 all arrays are now page aligned in memory, to be
used with SIMD instructions (e.g. fftw library). As a side effect,
arrays created with a previous version of SharedArray aren't
compatible with the new version (the location of the metadata
changed). Save your work before upgrading.

## Contact

This package is hosted on [GitLab](https://gitlab.com) at:
<https://gitlab.com/tenzing/shared-array>

Packages are also available on PyPi at:
<https://pypi.python.org/pypi/SharedArray>

For bug reports, feature requests, suggestions, patches and everything
else related to SharedArray, feel free to raise issues on the
[project page](https://gitlab.com/tenzing/shared-array). You can also
contact the maintainer directly by email at <mat@parad0x.org>.
