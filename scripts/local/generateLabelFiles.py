#! /usr/bin/python

import sys
import commands
import os

meetings = sys.argv[1]
fin = open(meetings)
annotations_dir = "/home/chengzhu/work/SpeechCorpus/ami_dir/segments/"
audio_dir = "/home/chengzhu/work/SpeechCorpus/ami_dir/amicorpus/"
file_dump = "./tmp/"
fjobs = open("./xml2txt.jobs",'w')
flabels = open("./seg2labels.jobs",'w')
for i in fin:
    session = i.strip()
    
    # 1. Generate txt files from segment xml:
    shell_cmd = "ls "+annotations_dir+session+"*"
    p = commands.getstatusoutput(shell_cmd)
    files = p[1].split('\n')
    fout = open(file_dump+"/segmentTXT_"+session+".txt",'w')
    for j in files:
        base_name = j.split('/')[-1].split('.xml')[0]
        txt_name = file_dump+base_name+'.txt'
        shell_cmd = "bash local/segXML2TXT.sh "+j+" "+txt_name
        fjobs.write(shell_cmd+'\n')
        fout.write(txt_name+"\n")
    shell_cmd = "ls "+audio_dir+session+"/audio/*.wav"
    p = commands.getstatusoutput(shell_cmd)
    wavfile = p[1].strip()
    fout.write(wavfile+"\n")
    fout.close()
    
    shell_cmd = ". ~/.bashrc;python local/seg2frame.py "+ \
                file_dump+"/segmentTXT_"+session+".txt "+file_dump+"/labels_"+session+".txt 0"
    flabels.write(shell_cmd+"\n")
flabels.close()
fjobs.close()

os.system("bash xml2txt.jobs")
os.system("bash seg2labels.jobs")
