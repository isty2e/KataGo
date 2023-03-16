#!/usr/bin/python3
import argparse
import datetime
import json
import multiprocessing
import os
import shutil
import sys
import time
import zipfile

import dateutil.parser
import numpy as np


def get_numpy_npz_headers(filename):
    with zipfile.ZipFile(filename) as z:
        wasbad = False
        # numrows = 0
        npzheaders = {}
        for subfilename in z.namelist():
            npyfile = z.open(subfilename)
            try:
                version = np.lib.format.read_magic(npyfile)
            except ValueError:
                wasbad = True
                print(f"WARNING: bad file, skipping it: {filename} (bad array {subfilename})")
            else:
                (shape, is_fortran, dtype) = np.lib.format._read_array_header(npyfile, version)
                npzheaders[subfilename] = (shape, is_fortran, dtype)
        if wasbad:
            return None
        return npzheaders


def is_temp_npz_like(filename):
    return "_" in filename


def summarize_dir(dirpath):
    filenames = [filename for filename in os.listdir(dirpath) if filename.endswith(".npz")]

    num_rows_this_dir = 0
    filename_mtime_num_rowss = []
    for filename in filenames:
        filepath = os.path.join(dirpath, filename)
        mtime = os.path.getmtime(filepath)

        # Files that look like they are temp files should be recorded and warned
        if is_temp_npz_like(filename):
            print("WARNING: file looks like a temp file: ", filepath)
            filename_mtime_num_rowss.append((filename, mtime, None))
            continue

        try:
            npheaders = get_numpy_npz_headers(filepath)
        except PermissionError:
            print("WARNING: No permissions for reading file: ", filepath)
            filename_mtime_num_rowss.append((filename, mtime, None))
            continue
        except zipfile.BadZipFile:
            print("WARNING: Bad zip file: ", filepath)
            filename_mtime_num_rowss.append((filename, mtime, None))
            continue

        if npheaders is None or len(npheaders) <= 0:
            print("WARNING: bad npz headers for file: ", filepath)
            filename_mtime_num_rowss.append((filename, mtime, None))
            continue

        (shape, is_fortran, dtype) = npheaders["binaryInputNCHWPacked"]
        num_rows = shape[0]
        num_rows_this_dir += num_rows

        filename_mtime_num_rowss.append((filename, mtime, num_rows))

    print("Summarizing new dir with %d rows: %s" % (num_rows_this_dir, dirpath), flush=True)
    return (dirpath, filename_mtime_num_rowss, num_rows_this_dir)


class TimeStuff(object):
    def __init__(self, taskstr):
        self.taskstr = taskstr

    def __enter__(self):
        print(f"Beginning: {self.taskstr}", flush=True)
        self.t0 = time.time()

    def __exit__(self, exception_type, exception_val, trace):
        self.t1 = time.time()
        print(
            f"Finished: {self.taskstr} in {str(self.t1 - self.t0)} seconds",
            flush=True,
        )
        return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Shuffle data files")
    parser.add_argument("dirs", metavar="DIR", nargs="+", help="Directories of training data files")
    parser.add_argument(
        "-old-summary-file-to-assume-correct", required=False, help="Summary json file for directory contents"
    )
    parser.add_argument("-new-summary-file", required=True, help="Summary json file for directory contents")
    parser.add_argument(
        "-num-parallel-processes", required=False, type=int, help="Number of parallel processes to use, default 4"
    )

    args = parser.parse_args()
    dirs = args.dirs
    old_summary_file_to_assume_correct = args.old_summary_file_to_assume_correct
    new_summary_file = args.new_summary_file

    num_processes = 4
    if args.num_parallel_processes is not None:
        num_processes = args.num_parallel_processes

    summary_data_by_dirpath = {}
    if old_summary_file_to_assume_correct is not None and os.path.exists(old_summary_file_to_assume_correct):
        with TimeStuff(f"Loading {old_summary_file_to_assume_correct}"):
            with open(old_summary_file_to_assume_correct) as fp:
                summary_data_by_dirpath = json.load(fp)

    dirs_to_handle = []
    with TimeStuff("Finding files"):
        for d in dirs:
            for path, dirnames, filenames in os.walk(d, followlinks=True):
                had_no_dirnames = len(dirnames) == 0
                i = 0
                while i < len(dirnames):
                    dirname = dirnames[i]
                    dirpath = os.path.normpath(os.path.join(path, dirname))
                    if dirpath in summary_data_by_dirpath:
                        del dirnames[i]
                        i -= 1
                    elif dirname == "tdata":
                        del dirnames[i]
                        i -= 1
                        dirs_to_handle.append(dirpath)
                    else:
                        parseddate = None
                        try:
                            parseddate = dateutil.parser.parse(dirname)
                        except ValueError:
                            parseddate = None
                        if parseddate is not None and parseddate < datetime.datetime.now() - datetime.timedelta(
                            days=2.0
                        ):
                            del dirnames[i]
                            i -= 1
                            dirs_to_handle.append(dirpath)

                    i += 1

    with TimeStuff("Parallel summarizing %d dirs" % len(dirs_to_handle)):
        with multiprocessing.Pool(num_processes) as pool:
            results = pool.map(summarize_dir, dirs_to_handle)

    num_total_rows = 0
    with TimeStuff("Merging %d results" % len(results)):
        for result in results:
            if result is None:
                continue
            (dirpath, filename_mtime_num_rowss, num_rows_this_dir) = result
            num_total_rows += num_rows_this_dir
            summary_data_by_dirpath[os.path.abspath(dirpath)] = filename_mtime_num_rowss

    if (
        not dirs_to_handle
        and old_summary_file_to_assume_correct is not None
        and os.path.exists(old_summary_file_to_assume_correct)
    ):
        shutil.copy(old_summary_file_to_assume_correct, new_summary_file)
        print("Not writing any new summary, no results, just copying old file")
    else:
        with TimeStuff("Writing result"):
            with open(new_summary_file, "w") as fp:
                json.dump(summary_data_by_dirpath, fp)
        print("Summary file written adding %d additional rows: %s" % (num_total_rows, new_summary_file), flush=True)

    print("Done computing new summary", flush=True)
    sys.stdout.flush()
