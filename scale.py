import sys


def displayTriads (scale):

	# for ranges from C2 to C5
	for i in range (2, 6):

		# create triad chords for each note of the scale
		for index in range (0, 7):

			string  = scale [index + 0][0] + str (scale [index + 0][1] + i) + ' '		# tonique
			string += scale [index + 2][0] + str (scale [index + 2][1] + i) + ' '		# tierce
			string += scale [index + 4][0] + str (scale [index + 4][1] + i) + ' '		# quinte

			print (string)

	return



#####################
# BEGINNING OF MAIN #
#####################

notes = [['C',0],['C#',0],['D',0],['D#',0],['E',0],['F',0],['F#',0],['G',0],['G#',0],['A',0],['A#',0],['B',0],
['C',1],['C#',1],['D',1],['D#',1],['E',1],['F',1],['F#',1],['G',1],['G#',1],['A',1],['A#',1],['B',1],
['C',2],['C#',2],['D',2],['D#',2],['E',2],['F',2],['F#',2],['G',2],['G#',2],['A',2],['A#',2],['B',2]]


# create minor scale for each note
print ('MINOR')
for index in range (0, 12):

	scale = []
	scale.append (notes[index])				# tonique
	scale.append (notes[(index + 2)])		# seconde
	scale.append (notes[(index + 3)])		# tierce mineure
	scale.append (notes[(index + 5)])		# quarte
	scale.append (notes[(index + 7)])		# quinte
	scale.append (notes[(index + 8)])		# sixte mineure
	scale.append (notes[(index + 10)])		# septième mineure
	scale.append (notes[(index + 12)])		# tonique
	scale.append (notes[(index + 14)])		# seconde
	scale.append (notes[(index + 15)])		# tierce mineure
	scale.append (notes[(index + 17)])		# quarte
	scale.append (notes[(index + 19)])		# quinte
	scale.append (notes[(index + 20)])		# sixte mineure
	scale.append (notes[(index + 22)])		# septième mineure

	print (scale)
	displayTriads (scale)

# create major scale for each note
print ('MAJOR')
for index in range (0, 12):

	scale = []
	scale.append (notes[index])				# tonique
	scale.append (notes[(index + 2)])		# seconde
	scale.append (notes[(index + 4)])		# tierce majeure
	scale.append (notes[(index + 5)])		# quarte
	scale.append (notes[(index + 7)])		# quinte
	scale.append (notes[(index + 9)])		# sixte majeure
	scale.append (notes[(index + 11)])		# septième majeure
	scale.append (notes[(index + 12)])		# tonique
	scale.append (notes[(index + 14)])		# seconde
	scale.append (notes[(index + 16)])		# tierce majeure
	scale.append (notes[(index + 17)])		# quarte
	scale.append (notes[(index + 19)])		# quinte
	scale.append (notes[(index + 21)])		# sixte majeure
	scale.append (notes[(index + 23)])		# septième majeure

	print (scale)
	displayTriads (scale)
