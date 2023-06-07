# SONGIFY : convert a song from CSV to data for Picopanion
#
# usage : songify number_of_songs_to_convert
#
# here is the file format of a song file (song.h)
#
# index is in UNSIGNED INT 16
# | 00 |				number of songs in the data
# | 01 |				pointer000 (index) to song 000 data
# | 02 |				pointer001 (index) to song 001 data
# | 03 |				pointer002 (index) to song 002 data
# | 04 |				pointer003 (index) to song 003 data
#
# | pointer000 |		start of song 000
# | 00         |		number of channels used in the song (should not be > max number of channels, ie. 8)
# | 01         |		number of steps in the song
# | 02         |		instrument number for channel 0
# | 03         |		instrument number for channel 1
# | ..         |
# | 02 + n     |		instrument number for channel n
# | p          |		STEP 0, channel 0
# | p + 01     |		STEP 0, channel 1
# | p + 02     |		STEP 0, channel 2
# | ..         |
# | p + n      |		STEP 0, channel n
# | p + n + 1  |		STEP 0, pad number (from 0x0 to 0x77)
# | p + n + 2  |		STEP 0, pad colour
# | q          |		STEP 1, channel 0
# | q + 01     |		STEP 1, channel 1
# | q + 02     |		STEP 1, channel 2
# | ..         |
# | q + n      |		STEP 1, channel n
# | q + n + 1  |		STEP 1, pad number (from 0x0 to 0x77)
# | q + n + 2  |		STEP 1, pad colour
#
#
# | pointer001 |		start of song 001
# | 00         |		number of channels used in the song (should not be > max number of channels, ie. 8)
# | 01         |		number of steps in the song
# | 02         |		instrument number for channel 0
# | 03         |		instrument number for channel 1
# | ..         |
# | 02 + n     |		instrument number for channel n
# | p          |		STEP 0, channel 0
# | p + 01     |		STEP 0, channel 1
# | p + 02     |		STEP 0, channel 2
# | ..         |
# | p + n      |		STEP 0, channel n
# | p + n + 1  |		STEP 0, pad number (from 0x0 to 0x77)
# | p + n + 2  |		STEP 0, pad colour
# | q          |		STEP 1, channel 0
# | q + 01     |		STEP 1, channel 1
# | q + 02     |		STEP 1, channel 2
# | ..         |
# | q + n      |		STEP 1, channel n
# | q + n + 1  |		STEP 1, pad number (from 0x0 to 0x77)
# | q + n + 2  |		STEP 1, pad colour
#


import os
import sys
import csv


notes = {'':0, 'END':0xFFFF, 'BASS':500, 'SNARE':6000, 'HAT':20000, 'A0':27.5, 'A#0':29.135, 'BB0':29.135, 'B0':30.868, 'C1':32.703, 'C#1':34.648, 'DB1':34.648, 'D1':36.708, 'D#1':38.89, 'EB1':38.89, 'E1':41.203, 'F1':43.653, 'F#1':46.249, 'GB1':46.249, 'G1':49, 'G#1':51.913, 'AB1':51.913, 'A1':55, 'A#1':58.271, 'BB1':58.271, 'B1':61.735, 'C2':65.406, 'C#2':69.296, 'DB2':69.296, 'D2':73.416, 'D#2':77.781, 'EB2':77.781, 'E2':82.407, 'F2':87.307, 'F#2':92.499, 'GB2':92.499, 'G2':98, 'G#2':103.83, 'AB2':103.83, 'A2':110, 'A#2':116.54, 'BB2':116.54, 'B2':123.47, 'C3':130.81, 'C#3':138.59, 'DB3':138.59, 'D3':146.83, 'D#3':155.56, 'EB3':155.56, 'E3':164.81, 'F3':174.61, 'F#3':184.99, 'GB3':184.99, 'G3':195.99, 'G#3':207.65, 'AB3':207.65, 'A3':220, 'A#3':233.08, 'BB3':233.08, 'B3':246.94, 'C4':261.62, 'C#4':277.18, 'DB4':277.18, 'D4':293.66, 'D#4':311.12, 'EB4':311.12, 'E4':329.62, 'F4':349.22, 'F#4':369.99, 'GB4':369.99, 'G4':392, 'G#4':415.3, 'AB4':415.3, 'A4':440, 'A#4':466.16, 'BB4':466.16, 'B4':493.88, 'C5':523.25, 'C#5':554.37, 'DB5':554.37, 'D5':587.32, 'D#5':622.25, 'EB5':622.25, 'E5':659.26, 'F5':698.45, 'F#5':739.99, 'GB5':739.99, 'G5':783.99, 'G#5':830.61, 'AB5':830.61, 'A5':880, 'A#5':932.32, 'BB5':932.32, 'B5':987.77, 'C6':1046.5, 'C#6':1108.7, 'DB6':1108.7, 'D6':1174.7, 'D#6':1244.5, 'EB6':1244.5, 'E6':1318.5, 'F6':1396.9, 'F#6':1480, 'GB6':1480, 'G6':1568, 'G#6':1661.2, 'AB6':1661.2, 'A6':1760, 'A#6':1864.7, 'BB6':1864.7, 'B6':1975.5, 'C7':2093, 'C#7':2217.5, 'DB7':2217.5, 'D7':2349.3, 'D#7':2489, 'EB7':2489, 'E7':2637, 'F7':2793.8, 'F#7':2960, 'GB7':2960, 'G7':3136, 'G#7':3322.4, 'AB7':3322.4, 'A7':3520, 'A#7':3729.3, 'BB7':3729.3, 'B7':3951.1, 'C8':4186} 
instr = {'melody':0, 'rhythm':1, 'drum':2, 'hi-hat':3, 'hihat':3, 'bass':4, 'piano':5, 'reed':6, 'pluckedguitar':7, 'violin':8,}
color = {'':'0x00', 'green':'0x3C', 'red':'0x0F', 'amber':'0x3F', 'yellow':'0x3E', 'orange':'0x2F'}


def processSong (fileName):

	# variables definition
	instruments = []
	steps = []
	result = []

	with open(fileName+'.csv', newline='', encoding='utf-8-sig') as songFile:
		song = csv.reader (songFile, delimiter=';', quotechar='\"')
		# get list of instruments
		instruments = song.__next__()
		# remove the last 2 elements from the instrument list (these are pad num and color)
		del instruments[-1]
		del instruments[-1]

		# get each step of the song
		for row in song:
			steps.append (row)
	# close file
	songFile.close ()

	# put number of instruments and number of steps in the result
	result.append (len (instruments))
	result.append (len (steps))
	
	# change instrument name with instrument number and add to the result
	for i in instruments:
		result.append (instr[i])

	# process step by step
	for i in steps:
		j = i[:len (i) - 2]
		for note in j:
			note = note.upper ()
			result.append (round(notes[note]))

		# add pad number to the result
		result.append (i[len (i) - 2])

		# change pad color name with hex value and add to the result
		result.append (color [i[len (i) - 1]])

	return result

######
# MAIN
######

finalResult = []
index = []
	
# check if number of files to process
if len (sys.argv) >= 2:
	numberOfFiles = int (sys.argv[1])

	# 1st int is number of files
	finalResult.append (numberOfFiles)
	
	# room for start index of files
	for i in range (numberOfFiles):
		finalResult.append (0)

	# index contains the list of index of songs
	# start with index of song 000
	index.append (len(finalResult))

	# process song one after the other
	for i in range (numberOfFiles):
		# pad with 0, max 3 chars
		finalResult.extend (processSong (str ('%0*d' % (3,i))))
		index.append (len (finalResult))

	# copy indexes to finalResult
	for i in range (numberOfFiles):
		# leave 1 byte more for number of files
		finalResult [i+1] = index [i]

	# format final result string
	s = 'const uint16_t song ['
	s += str (len(finalResult))
	s += '] = {'
	for i in finalResult:
		s += str (i)
		s += ','
	# remove trailing ','
	s = s[:-1]
	s = s + '};\n'

	print (s)
	print ('\nwriting to file...\n')
	with open("song.h", "wt") as f:
		print (s, file=f)
	f.close ()
	
else:
	print ("\nusage : songify.py number_of_songs_to_convert\n")


