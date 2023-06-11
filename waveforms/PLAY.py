import csv
import random
from math import fmod
import numpy as np
from matplotlib import pyplot as plt
from argparse import ArgumentParser
import wavio



#constants
SAMPLERATE = 48000.0
# frequencies from C4 to C5
frequencies = [262, 294, 330, 349, 392, 440, 494, 523] 

def NormalizeData (data):
	# normalize data between -1 and 1
    return (((data - np.min(data)) / (np.max(data) - np.min(data)) * 2.0) - 1.0) 



#####################
# BEGINNING OF MAIN #
#####################


if __name__ == "__main__":
	parser = ArgumentParser()
	parser.add_argument("-f", "--file", dest="filename", help="CSV FILE to read", metavar="FILE")
	parser.add_argument("-d", "--delimiter", dest="d", default=';',type=str, help="delimiter in csv")
	parser.add_argument("-o", "--output", dest="output_file", default=None, help="output FILE to write to, default=stdout", metavar="FILE")
	args = parser.parse_args()
	if args.d == 't':
		args.d = '\t'
	elif args.d == 'n':
		args.d = '\n'

	# open sample and store it to a list
	sample = []
	dispSample = []
	sndSample = []
	with open(args.filename, newline='') as sampleFile:
		smpl = csv.reader (sampleFile, delimiter = args.d, quotechar='\"')
		# skip 1st line (header line)
		#smpl.__next__()

		for row in smpl:
			sample.append (row)
	# close file
	sampleFile.close ()

	# create a *.h file to include to a project
	# create start of line
	res = 'const int16_t ' + args.output_file + "_waveform [256] = {"
	with open(args.output_file + ".h", "wt", newline='') as sampleFile:
		for itms in sample:
			res = res + itms [1] + ","
		# remove last ","
		res = res [:-1]
		res += '};\n'
		# write result in file
		sampleFile.write (res)
	
	# close file
	sampleFile.close ()
	

	# In parallel, create a 3 seconds wav file we are going to play to have a idea of the sound
	# we will play C4 to C5 notes
	# That is, in 0.5 seconds, we will have (taking A 440Hz as an example):
	# 0.5 * samplerate samples to play
	# these samples shall come from a wave with 0.5 * 440 periods
	# "Rule of 3", règle de trois !
	# that is (remove the 0.5 in both terms): "samplerate" ticks = 440 periods of wave = 440 * 256 samples
	# that is: 1 tick = 440 * 256 / samplerate
	# so if our time index is X ticks, then we should get sample number (X * 440 * 256 / samplerate) % 256 ; modulus 256 is to make sure we don't go over 256
	rate = int (SAMPLERATE)
	notelength = 0.5
	for j in frequencies:
		for i in range (int (rate * notelength)):
			index = int (i * j * 256 / rate) % 256
			s = float (sample [index][1]) / 32767.0		# s is between -1.0 and 1.0
			sndSample.append (s)
	# C chord
	for i in range (int (rate * notelength)):
		index1 = int (i * frequencies[0] * 256 / rate) % 256
		index3 = int (i * frequencies[2] * 256 / rate) % 256
		index5 = int (i * frequencies[4] * 256 / rate) % 256
		s1 = float (sample [index1][1]) / 32767.0		# s1 is between -1.0 and 1.0
		s3 = float (sample [index3][1]) / 32767.0		# s3 is between -1.0 and 1.0
		s5 = float (sample [index5][1]) / 32767.0		# s5 is between -1.0 and 1.0
		s = (s1 + s3 + s5) / 3.0
		sndSample.append (s)
	# D chord
	for i in range (int (rate * notelength)):
		index1 = int (i * frequencies[1] * 256 / rate) % 256
		index3 = int (i * frequencies[3] * 256 / rate) % 256
		index5 = int (i * frequencies[5] * 256 / rate) % 256
		s1 = float (sample [index1][1]) / 32767.0		# s1 is between -1.0 and 1.0
		s3 = float (sample [index3][1]) / 32767.0		# s3 is between -1.0 and 1.0
		s5 = float (sample [index5][1]) / 32767.0		# s5 is between -1.0 and 1.0
		s = (s1 + s3 + s5) / 3.0
		sndSample.append (s)
	# E chord
	for i in range (int (rate * notelength)):
		index1 = int (i * frequencies[2] * 256 / rate) % 256
		index3 = int (i * frequencies[4] * 256 / rate) % 256
		index5 = int (i * frequencies[6] * 256 / rate) % 256
		s1 = float (sample [index1][1]) / 32767.0		# s1 is between -1.0 and 1.0
		s3 = float (sample [index3][1]) / 32767.0		# s3 is between -1.0 and 1.0
		s5 = float (sample [index5][1]) / 32767.0		# s5 is between -1.0 and 1.0
		s = (s1 + s3 + s5) / 3.0
		sndSample.append (s)
	# F chord
	for i in range (int (rate * notelength)):
		index1 = int (i * frequencies[3] * 256 / rate) % 256
		index3 = int (i * frequencies[5] * 256 / rate) % 256
		index5 = int (i * frequencies[7] * 256 / rate) % 256
		s1 = float (sample [index1][1]) / 32767.0		# s1 is between -1.0 and 1.0
		s3 = float (sample [index3][1]) / 32767.0		# s3 is between -1.0 and 1.0
		s5 = float (sample [index5][1]) / 32767.0		# s5 is between -1.0 and 1.0
		s = (s1 + s3 + s5) / 3.0
		sndSample.append (s)


	sound = NormalizeData (sndSample)
	wavio.write(args.output_file + ".wav", sound, rate, sampwidth=3)


	# create new sample to display by merging 3 times the original sample 
	dispSample = sample + sample + sample
	x = []
	y = []

	# display wave
	# get X and Y from list read from CSV
	for i in range (len(dispSample)):
		x.append (i)
		y.append (int (dispSample [i][1]))



	plt.rcParams["figure.figsize"] = [7.50, 3.50]
	plt.rcParams["figure.autolayout"] = True
	plt.plot(x, y, color = 'green')
	plt.savefig(args.output_file + ".png")
