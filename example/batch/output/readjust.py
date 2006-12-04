#!/usr/bin/python

import os, sys;
from optparse import OptionParser;

def readjust(filename):
    """Due to a new simbatch functionnality,
    time between simbatch are delayed. This script
    readjust the simbatch output file to compare 
    them on the same base than oar .out"""
    nb_tasks = 0
    submit_times = []
    start_times = []
    end_times = []
    duration = []

    if not os.path.isfile(filename):
        raise IOError, "'%s' doesn't exist!" % filename
    
    data = {}
    for line in open(filename):
        if line.startswith('e', 0, 1) or line.startswith('#', 0, 1):
            pass
        else:
            temp = line[4:].split()
            data[temp[0]] = [float(v) for v in temp[1:]]
            del temp

    nb_tasks = len(data.keys())
    for key in xrange(1, nb_tasks + 1):
        submit_times.append(data[str(key)][0])
        start_times.append(data[str(key)][1])
        end_times.append(data[str(key)][2])
        duration.append(data[str(key)][3])
    del data

    datafile = open(filename, 'w')
    datafile.write("# Simbatch result for %s\n" % os.path.basename(filename))
    start_value = submit_times[0]
    for key in xrange(nb_tasks):
        submit_times[key] -= start_value
        start_times[key] -= start_value
        end_times[key] -= start_value

        datafile.write("%s\t%f\t%f\t%f\t%f\n" % ('task'+ str(key+1),
                                                  submit_times[key],
                                                  start_times[key],
                                                  end_times[key],
                                                  duration[key]))
    datafile.close()


def options():
    "Handle options from command line"
    parser = OptionParser()

    parser.add_option("-f", "--file", type="string",
                      help="file to readjust")
    parser.add_option("-r", "--rep", type="string",
                      help="readjust all files in the rep")

    (options, args) = parser.parse_args()
    # print options
    if not options.file and not options.rep:
        parser.print_help()
        sys.exit(1)

    return options, args


if __name__ == '__main__':

    (o, args) = options()
    if o.file is not None:
        try:
            readjust(o.file)
        except IOError:
            print 'Error: %s' % "can't open file %s!" % o.file

    if o.rep is not None:
        if not os.path.isdir(o.rep):
            print 'Error: %s' % "%s is not a directory!" % o.rep
            sys.exit(2)

        for filename in os.listdir(o.rep):
            readjust(o.rep + filename)

