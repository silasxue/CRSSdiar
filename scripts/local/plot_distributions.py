#! /usr/bin/python 

import sys
import pylab 

def read_scores(fin):
    out_list = []
    for i in fin:
        line_list = i.strip().split(' ')
        score = float(line_list[-1])
        out_list.append(score)
    return out_list

true_scores = sys.argv[1]
false_scores = sys.argv[2]

ftrue = open(true_scores)
tr = read_scores(ftrue)
ftrue.close()

ffalse = open(false_scores)
fa = read_scores(ffalse)
ffalse.close()


pylab.hist(fa,100)
pylab.hist(tr,100)
pylab.show()

