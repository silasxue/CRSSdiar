#! /usr/bin/python 

import sys
import pylab 
import numpy as np

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


x1 = np.array(fa)
x1 = x1.reshape((len(fa),1))
x2 = np.array(tr)
x2 = x2.reshape((len(tr),1))
x  = np.hstack((x1[:min(len(fa),len(tr))],x2[:min(len(fa),len(tr))]))
labels = ['non-target', 'target']
pylab.hist(x,100, normed=1, color=['red','chartreuse'], histtype='bar', label=labels)
pylab.legend(prop={'size': 10})
pylab.show()
