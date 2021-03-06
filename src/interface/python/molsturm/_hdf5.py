#!/usr/bin/env python3
## vi: tabstop=4 shiftwidth=4 softtabstop=4 expandtab
## ---------------------------------------------------------------------
##
## Copyright (C) 2017 by the molsturm authors
##
## This file is part of molsturm.
##
## molsturm is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published
## by the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## molsturm is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with molsturm. If not, see <http://www.gnu.org/licenses/>.
##
## ---------------------------------------------------------------------

import h5py
import numpy as np
from os.path import basename


def __emplace_ndarray(keyval, group, typ, **kwargs):
    dset = group.create_dataset(keyval[0], data=keyval[1], **kwargs)
    dset.attrs["type"] = "ndarray"


def __extract_ndarray(dataset):
    # dtype = __dtype_array_from_hdf5(dataset.dtype)
    arr = np.empty(dataset.shape, dtype=dataset.dtype)
    dataset.read_direct(arr)
    return (basename(dataset.name), arr)


def __emplace_listlike(keyval, group, typ, **kwargs):
    dtype = None
    # Usually the heuristic for doing the conversion is pretty
    # good here, but there are some exceptions.
    if all(isinstance(v, str) for v in keyval[1]):
        dtype = h5py.special_dtype(vlen=str)

    ary = np.array(keyval[1], dtype=dtype)
    dset = group.create_dataset(keyval[0], data=ary, **kwargs)
    dset.attrs["type"] = "list"


def __extract_listlike(dataset):
    key, arr = __extract_ndarray(dataset)
    return (key, arr.tolist())


def __emplace_none(keyval, group, typ, **kwargs):
    dset = group.create_dataset(keyval[0], data=h5py.Empty("f"), **kwargs)
    dset.attrs["type"] = "none"


def __extract_none(dataset):
    return (basename(dataset.name), None)


# Type transformations for scalar types
# If type not found here, we have an error
# in the direction python -> hdf5, else we ignore it.
__scalar_transform = [
    (str,     h5py.special_dtype(vlen=str)),
    (bool,    np.dtype("b1")),
    (complex, np.dtype("c16")),
    (float,   np.dtype("f8")),
    (int,     np.dtype("int64")),
]


def __emplace_scalar(keyval, group, typ, **kwargs):
    dtype = None  # Indicate no target type found
    for t in __scalar_transform:
        if isinstance(keyval[1], t[0]):
            dtype = t[1]
            break
    if dtype is None:
        raise TypeError("Encountered unknown data type '" +
                        str(type(keyval[1])) + "'")

    dset = group.create_dataset(keyval[0], data=keyval[1],
                                dtype=dtype, **kwargs)
    dset.attrs["type"] = "scalar"


def __extract_scalar(dataset):
    dtype = None  # Target type to transform to
    for t in __scalar_transform:
        if dataset.dtype == t[1]:
            dtype = t[0]
            break

    if dataset.shape == ():  # i.e. HDF5 scalar
        ret = dataset.value
    else:
        ret = dataset[0]

    if dtype is not None:
        ret = dtype(ret)
    return (basename(dataset.name), ret)


def __extract_dataset(dataset):
    """Select extractor based on the type attribute and use that
       to make the proper key-value pair out of the dataset
    """
    if "type" not in dataset.attrs:
        if dataset.shape == ():
            return __extract_scalar(dataset)  # Treat as scalar
        else:
            return __extract_ndarray(dataset)  # Treat as array
    else:
        # Use type attribute to distinguish what should happen
        tpe = dataset.attrs["type"]
        return {
            "scalar":   __extract_scalar,
            "none":     __extract_none,
            "ndarray":  __extract_ndarray,
            "list":     __extract_listlike,
            "tuple":    __extract_listlike,
        }[tpe](dataset)


def __emplace_key_value(kv, group, **kwargs):
    """
    Emplace a single key-value pair in the group.

    What precisely happends depends on the type of the value
    to emplace.
    """

    def __emplace_dict_inner(kv, group, typ, **kwargs):
        subgroup = group.create_group(kv[0])
        emplace_dict(kv[1], subgroup)

    emplace_map = [
        (np.ndarray,   __emplace_ndarray),
        (type(None),   __emplace_none),
        (list,         __emplace_listlike),
        (tuple,        __emplace_listlike),
        (dict,         __emplace_dict_inner),
    ]

    for (typ, emplace) in emplace_map:
        if isinstance(kv[1], typ):
            try:
                emplace(kv, group, typ, **kwargs)
            except TypeError as e:
                raise TypeError("Error with key '" + kv[0] + "': " + str(e))
            return

    # Fallback: Assume value is a simple scalar type
    try:
        __emplace_scalar(kv, group, typ, **kwargs)
    except TypeError as e:
        raise TypeError("Error with key '" + kv[0] + "': " + str(e))

#
# High-level routines
#


def emplace_dict(d, group, **kwargs):
    """
    Emplace a python dictionary "d" into the HDF5 group "group"
    using the kwargs to create all neccessary datasets.
    """
    for kv in d.items():
        __emplace_key_value(kv, group, **kwargs)


def extract_group(group):
    # Recursively extract all groups:
    ret = {basename(v.name): extract_group(v) for v in group.values()
           if isinstance(v, h5py.Group)}

    # Now deal with all datasets
    ret.update([__extract_dataset(v) for v in group.values()
                if isinstance(v, h5py.Dataset)])

    if not all(isinstance(v, (h5py.Dataset, h5py.Group)) or v is None
               for v in group.values()):
        raise ValueError("Encountered object in h5py which is neither "
                         "a Group nor a Dataset")
    return ret
