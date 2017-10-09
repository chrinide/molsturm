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

import glob
import molsturm
import molsturm.posthf
import molsturm.yaml_utils
import os
import yaml


SKIPPED_SINCE_DONE = "Skipped since already done."


def build_input_params():
    """Gather a dictionary of all input parameters"""
    dir_of_this_script = os.path.dirname(os.path.abspath(__file__))

    # Build the list of input data
    inputs = dict()
    for f in glob.iglob(os.path.join(dir_of_this_script, "*.in.yaml")):
        fn, _ = os.path.splitext(f)
        key = fn[:-3]

        with open(f, "r") as stream:
            inputs[key] = yaml.safe_load(stream)
    return inputs


def print_file_start(filename):
    print("#")
    print("# " + os.path.basename(filename))


def print_status(key, message):
    print("{0:15s} {1}".format(key + ":", message))


# Cache of calculations we already did
calculation_scf_cache = dict()


def run_scf_calculation(name, scfparams):
    if name not in calculation_scf_cache:
        print_status("run_hf", "Running HF calculation")
        calculation_scf_cache[name] = molsturm.self_consistent_field(scfparams)
    return calculation_scf_cache[name]

# --------------------------------------------------------------------


def job_dump_yaml(name, scfparams, dump_params):
    """Run a full calculation and dump the result as a yaml file"""
    output = name + ".hf.yaml"
    if not os.path.exists(output):
        res = run_scf_calculation(name, scfparams)

        # Remove keys which are given by the parameters
        for key in dump_params.get("remove_keys", []):
            if key in res:
                del res[key]

        molsturm.dump_yaml(res, output)
    else:
        print_status("dump_yaml", message=SKIPPED_SINCE_DONE)


def job_dump_hdf5(name, scfparams, dump_params):
    """Run a full calculation and dump the result as a yaml file"""
    output = name + ".hf.hdf5"
    if not os.path.exists(output):
        res = run_scf_calculation(name, scfparams)

        # Remove keys which are given by the parameters
        for key in dump_params.get("remove_keys", []):
            if key in res:
                del res[key]

        molsturm.dump_hdf5(res, output)
    else:
        print_status("dump_hdf5", message=SKIPPED_SINCE_DONE)


def job_posthf_mp2(name, scfparams, mp_params):
    output = name + ".mp2.yaml"
    if not os.path.exists(output):
        hfres = run_scf_calculation(name, scfparams)
        print_status("posthf_mp2", "Running MP2")
        mp2 = molsturm.posthf.mp2(hfres, **mp_params)

        molsturm.yaml_utils.install_representers()
        with open(output, "w") as f:
            yaml.safe_dump(mp2, f)
    else:
        print_status("posthf_mp2", message=SKIPPED_SINCE_DONE)


def job_posthf_fci(name, scfparams, fci_params):
    output = name + ".fci.yaml"
    if not os.path.exists(output):
        hfres = run_scf_calculation(name, scfparams)
        print_status("posthf_fci", "Running Full-CI")
        fci = molsturm.posthf.fci(hfres, **fci_params)

        molsturm.yaml_utils.install_representers()
        with open(output, "w") as f:
            yaml.safe_dump(fci, f)
    else:
        print_status("posthf_fci", message=SKIPPED_SINCE_DONE)

# --------------------------------------------------------------------


def work_on_case(name, scfparams, jobs):
    print_file_start(name)
    for job in jobs:
        if isinstance(job, str):
            jobname = job
            jobparams = {}
        elif isinstance(job, dict):
            jobname, jobparams = job.popitem()
        else:
            raise SystemExit("Invalid entry in include list of type '" + type(job) +
                             "' found.")

        try:
            globals()["job_" + jobname](name, scfparams, jobparams)
        except KeyError:
            raise SystemExit("Unknown job name '" + jobname + "' in input file '" +
                             name + ".in.yaml'")


def main():
    inputs = build_input_params()
    for name in sorted(inputs):
        # Build the ScfParameters object to run the
        # SCF for producing the reference data.
        scfparams = molsturm.ScfParameters.from_dict(inputs[name]["input_parameters"])
        scfparams.normalise()

        # The include list of jobs which should be performed
        # on this input:
        include = inputs[name]["include"]

        work_on_case(name, scfparams, include)


if __name__ == "__main__":
    main()
