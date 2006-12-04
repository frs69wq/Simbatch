#!/usr/bin/env python

r"""
analyser -f file.out
analyser -[vw]c file1.out file2.out
analyser -r rep1/ rep2/
analyser -h

Analyse .out file and compare them following the option.
-f : analyse a file
-c : compare 2 files
-v : verbose mode (use it with -c)
-w : write the result (use it with -c)
-r : compare .out file having the same name in the 2 directories
"""
__author__ = "Jean-Sebastien Gay"
__date__ = "23 octobre 2006"
__version__ = "$Revision$"
__credits__ = "thanks to pylint for code cleaning"



import sys, os
import scipy

from optparse import OptionParser

class Parser(object):
    """Parser for .out file"""

    def __init__(self, filename):
        """ Convert a .out file in a Parser object """
        self.__filename = filename
        self.__nb_tasks = 0
        self.__submit_times = []
        self.__start_times = []
        self.__end_times = []

        if not os.path.isfile(filename):
            raise IOError, "'%s' doesn't exist!" % filename
        
        data = {}
        for line in open(filename):
            if line.startswith('e', 0, 1) or line.startswith('#', 0, 1):
                pass
            else:
                temp = line[4:].split()
                data[temp[0]] = [float(value) for value in temp[1:]]
                del temp

        self.__nb_tasks = len(data.keys())
        for key in xrange(1, self.__nb_tasks + 1):
            self.submit_times.append(data[str(key)][0])
            self.start_times.append(data[str(key)][1])
            self.end_times.append(data[str(key)][2])
        del data

    def get_flows(self):
        """Return the list of the tasks flow
        '() -> 'list """
        flows = []
        for i in xrange(self.nb_tasks):
            flows.append(self.end_times[i] - self.submit_times[i])
        return flows
    
    def get_makespan(self):
        """Return the list of the tasks makespan
        '() -> 'list """
        makespan = []
        for i in xrange(self.__nb_tasks):
            makespan.append(self.end_times[i] - self.start_times[i])
        return makespan

    def get_xp_makespan(self):
        """Return the makespan of the experience
        '() -> 'float """
        return max(self.__end_times)
        
    ### Properties ###
    def get_filename(self):
        """Property for the filename attribute"""
        return self.__filename
    
    def get_nb_tasks(self):
        """Property for the nb_tasks attribute"""
        return self.__nb_tasks

    def get_submit_times(self):
        """Property for the submit_times attribute"""
        return self.__submit_times

    def get_start_times(self):
        """Property for the start_times attribute"""
        return self.__start_times

    def get_end_times(self):
        """Property for the end_times attribute"""
        return self.__end_times
    
    # Ne marche qu'avec les new style class i.e. class(object):
    filename = property(fget=get_filename)
    nb_tasks = property(fget=get_nb_tasks)
    submit_times = property(fget=get_submit_times)
    start_times = property(fget=get_start_times)
    end_times = property(fget=get_end_times)


################## Other functions
def cmp_2_lists(list1, list2):
    """ Compare values from 2 lists 
        'list * 'list -> 'list
    """
    res = []
    for i in xrange(0, min([len(list1), len(list2)])):
        res.append((1. - (list1[i]/list2[i])) * 100)
    
    return res

def geometric_mean(values):
    "Compute the geometric mean of values in a list"
    try:
        power = 1./len(values)
        float_values = [float(val) ** power for val in values]
        gmean = 1.
        for val in float_values:
            gmean *= val
    except(ValueError):
        gmean = 0.

    return gmean

def analyse_list(list_values):
    "Compute some stats on a list"
    array = scipy.array(list_values)
    return min(list_values), max(list_values), scipy.stats.mean(array), \
           geometric_mean(list_values), scipy.stats.std(array)

def print_file_stats(parser):
    "Print some stats one a file"
    print "File : ", parser.filename
    print "\tNumber of tasks : ", parser.nb_tasks
    print "\tExperience makespan : ", parser.get_xp_makespan()
    print "\tExperience sumflow : ", sum(parser.get_flows())

def print_list_stats(stats):
    "Print some stats obtained from a list"
    print "Stats :"
    print "\tMin : ", stats[0]
    print "\tMax : ", stats[1]
    print "\tArithmetic mean : ", stats[2]
    print "\tGeometric mean : ", stats[3]
    print "\tStandard deviation : ", stats[4]

def print_data(start_time, error):
    "Print the .dat file : flow error following start_time"
    list_dat = []
    for i in xrange(len(start_time)):
        list_dat += [(start_time[i], error[i])]
        
    pos = [(start, err) for (start, err) in list_dat if (err >= 0)]
    neg = [(start, abs(err)) for (start, err) in list_dat if (err < 0)]

    print "Positive values :"
    for (start, error) in pos:
        if error > 2:
            print "\t%f\t%f **" % (start, error)
        else:
            print "\t%f\t%f" % (start, error)
    print "\nNegative values :" 
    for (start, error) in neg:
        print "\t%f\t%f" % (start, error)

def write_data(output, start_time, error):
    "Write a .dat file : flow error following start_time"
    list_dat = []
    for i in xrange(len(start_time)):
        list_dat += [(start_time[i], error[i])]
        
    pos = [(start, err) for (start, err) in list_dat if (err >= 0)]
    neg = [(start, abs(err)) for (start, err) in list_dat if (err < 0)]

    datafile = open(output, 'w')
    datafile.write("# Positive value 1 > 2\n")
    for (start, error) in pos:
        datafile.write("%f\t%f\n" % (start, error))
         
    # Gnuplot needs 2 blank line to make a block
    datafile.write("\n\n")
    datafile.write("# Negative value 1 < 2\n")
    for (start, error) in neg:
        datafile.write("%f\t%f\n" % (start, error))

    datafile.close()


################## Main 
def options():
    "Handle options from command line"
    parser = OptionParser()

    parser.add_option("-c", "--comp", type="string", nargs=2,
                      help="files to analyse and compare")
    parser.add_option("-f", "--file", type="string",
                      help="file to analyse")
    parser.add_option("-r", "--rep", type="string", nargs=2,
                      help="analyse all files in rep")
    parser.add_option("-v", "--verbose", action="store_true",
                      dest="verbose", default="false",
                      help="print more infos")
    parser.add_option("-w", "--write", action="store_true",
                      dest="write", default="false",
                      help="write a .dat file")

    (opt, args) = parser.parse_args()
    # print options
    if not opt.comp and not opt.file and not opt.rep:
        parser.print_help()
        sys.exit(1)
        
    return opt, args


def main():
    "Main programm"
    opt = options()[0]
   
    if opt.file is not None:
        # Analyse one file if it exists
        try:
            print_file_stats(Parser(opt.file))
        except IOError , error:
            print 'Error: %s' % str(error)
            sys.exit(2)

    if opt.comp is not None:
        # Compare 2 files if they exist
        try:
            parser1 = Parser(opt.comp[0])
            parser2 = Parser(opt.comp[1])
            res = cmp_2_lists(parser1.get_flows(), parser2.get_flows())
            print_file_stats(parser1)
            print_file_stats(parser2)
            print_list_stats(analyse_list(res))
            if opt.verbose is True:
                print ''
                print_data(parser1.start_times, res)
            if opt.write is True:
                write_data(os.path.basename(opt.comp[0])[:-4]
                            + '.dat', parser1.start_times, res)
                
            del res, parser1, parser2
        except IOError, error:
            print 'Error: %s' % str(error)
            sys.exit(2)

    if opt.rep is not None:
        # if bad rep, exit
        if not os.path.isdir(opt.rep[0]) or not os.path.isdir(opt.rep[1]):
            print 'Error: %s' % "%s or %s does'not exist" % (opt.rep[0], 
                                                             opt.rep[1])
            sys.exit(3)

        # analyse file that have same name in the 2 directories
        files = []
        stats = []
        for filename in os.listdir(opt.rep[0]):
            if os.path.isfile(opt.rep[1] + filename):
                files.append(filename)

        for fic in files:
            parser1 = Parser(opt.rep[0] + fic)
            parser2 = Parser(opt.rep[1] + fic)
            temp = cmp_2_lists(parser1.get_flows(), parser2.get_flows())
            stats += [analyse_list(temp)]
            del temp, parser1, parser2
            
        stats = [values[2] for values in stats]
        print_list_stats(analyse_list(stats))


if __name__ == "__main__":
    main()
