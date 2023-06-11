import random
from math import fmod
import numpy as np
from matplotlib import pyplot as plt
from argparse import ArgumentParser


#constants
SAMPLERATE = 256.0
PI = 3.141592653589793

# define wave class
class wave:

	# low level function for wave shape
	def square (angle):
		# determine if angle is negative, in which case we invert sample value compared to positive
		if angle < 0:
			negative = True
			angle *= (-1.0)
		else:
			negative = False

		# make sure angle is in range (0 , 2PI)
		angle = fmod (angle , (2 * PI))
        
		# determine sample value based on angle value
		if angle < PI:
			sample = 1.0
		else:
			sample = (-1.0)

		# if angle is negative, then invert value of sample
		if negative:
			sample *= (-1.0)
            
		return sample


	def triangle (angle):
		# determine if angle is negative, in which case we invert sample value compared to positive
		if angle < 0:
			negative = True
			angle *= (-1.0)
		else:
			negative = False

		# make sure angle is in range (0 , 2PI)
		angle = fmod (angle , (2 * PI))
        
		# determine sample value based on angle value
		# for this, we use the line equation y = ax+b
		if angle <= (PI / 2):
			a = (2 / PI)
			b = 0.0
		elif angle <= PI:
			a = -(2 / PI)
			b = 2.0
		elif angle <= (3 * PI / 2):
			a = -(2 / PI)
			b = 2.0	
		else:
			a = (2 / PI)
			b = -4.0
		
		sample = (a * angle) + b

		# if angle is negative, then invert value of sample
		if negative:
			sample *= (-1.0)
            
		return sample


	def saw1 (angle):
		# determine if angle is negative, in which case we invert sample value compared to positive
		if angle < 0:
			negative = True
			angle *= (-1.0)
		else:
			negative = False

		# make sure angle is in range (0 , 2PI)
		angle = fmod (angle , (2 * PI))
        
		# determine sample value based on angle value
		# for this, we use the line equation y = ax+b
		if angle <= PI:
			a = (1 / PI)
			b = 0.0
		else:
			a = (1 / PI)
			b = -2.0
		
		sample = (a * angle) + b

		# if angle is negative, then invert value of sample
		if negative:
			sample *= (-1.0)
            
		return sample


	def saw2 (angle):
		# determine if angle is negative, in which case we invert sample value compared to positive
		if angle < 0:
			negative = True
			angle *= (-1.0)
		else:
			negative = False

		# make sure angle is in range (0 , 2PI)
		angle = fmod (angle , (2 * PI))
        
		# determine sample value based on angle value
		# for this, we use the line equation y = ax+b
		if angle <= PI:
			a = -(1 / PI)
			b = 0.0
		else:
			a = -(1 / PI)
			b = 2.0
		
		sample = (a * angle) + b

		# if angle is negative, then invert value of sample
		if negative:
			sample *= (-1.0)
            
		return sample


	# define functions for each shape of wave
	waveShapes = {'sinus' : np.sin , 'square' : square , 'triangle' : triangle , 'saw1' : saw1 , 'saw2' : saw2 }


	# constructor
	def __init__(self , shape , frequency , amplitude , phase):

		self.shape = shape
		self.frequency = frequency
		self.period = 1.0 / frequency
		self.amplitude = amplitude
		self.phase = phase    

		return


	def readWave (self , tick):
  
		tickInSeconds = tick / SAMPLERATE
		# positionInWave indicates where (in seconds) we are in the period of the wave; it is < period
		positionInWave = fmod (tickInSeconds , self.period)
		
		# we can consider that during 1 period, we progress through a circle of 2PI radians; ie. within 1 period, we will go through all values of sin ()
		# we calculate the current angle we are doing based on our current position in the wave (compared to period), and adding the phase
		angle = ((positionInWave * 2 * PI) / self.period) + self.phase
        
		# determine sample value based on wave type
		sample = self.waveShapes.get (self.shape) (angle)
		# apply amplitude
		sample *= self.amplitude
        
		return sample



def NormalizeData (data):
	# normalize data between -1 and 1 (-0x7fff and 0x7fff)
    return ((((data - np.min(data)) / (np.max(data) - np.min(data)) * 2.0) - 1.0) * 0x7fff)


def write_file(filename, data):
	"""Simple file write, Converts to string if data object is not already"""
	if (type(data) == list) or (type(data) == tuple):
		data = '\n'.join(map(str,data))
	with open(filename, 'w') as f:
		f.write(data)
		
def Exit(exit_str):
	"""Exit with a message"""
	print(exit_str)
	input("Press enter to exit")
	raise SystemExit


#####################
# BEGINNING OF MAIN #
#####################

waveres = list ()
ondes = list ()


shapes = ('sinus' , 'square' , 'triangle' , 'saw1' , 'saw2')
ondes.append (wave ('sinus' , 2.0 , 0.5 , 0))
ondes.append (wave ('sinus' , 1.0 , 1 , 0))

rate = int (SAMPLERATE)	# samples per second
T = 1					# sample duration (seconds)

x = np.linspace (0 , rate * T , rate * T, False)
for i in x:
	res = 0.0
	for onde in ondes:
		res += onde.readWave (i)
	waveres.append (res)


if __name__ == "__main__":
	parser = ArgumentParser()
	parser.add_argument("-o", "--output", dest="output_file", default=None, help="output FILE to write to, default=stdout", metavar="FILE")
	parser.add_argument("-d", "--delimiter", dest="d", default=';',type=str, help="delimiter between xy output")
	args = parser.parse_args()
	if args.d == 't':
		args.d = '\t'
	elif args.d == 'n':
		args.d = '\n'

	# normalize and prepare output
	normres = NormalizeData (waveres)
	out = []
	j = 0
	for i in normres:
		out.append("{}{}{}".format(j, args.d, int (i)))
		j = j + 1

	out.append('')

	# write to file or stdout
	if args.output_file == None:
		print('\n'.join(map(str,out)))
	else:
		write_file(args.output_file, out)



