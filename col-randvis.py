#!/usr/bin/python3
import sys
from PIL import Image, ImageDraw
import numpy as np
from matplotlib import cm
import scipy.misc

fname = sys.argv[1]
x = 1700
y = 900
colnum = x*y
ffname = fname.split('.')[0]
dtcol = np.dtype('B')

def ColorMap(incol):
	calk = round((incol/255),2)
	if calk > 1:
		calk = 1
	if calk < 0:
		calk = 0
	mappedcol=cm.nipy_spectral(calk,1,1)
	return (mappedcol)

colimdata = np.fromfile(fname, dtype=dtcol, count=colnum)

newimdata=list()

image = Image.new('RGB', (x,y), color = 'black')
#counter = 0
#for xpos in range(x):
#	for ypos in range(y):
#		newimdata.append(ColorMap(colimdata[counter]))
#		counter+=1
for i in colimdata:
	newimdata.append(ColorMap(colimdata[i]))

image.putdata(newimdata)
image.save(ffname+"-col.png")

#for i in np.nditer(colimdata):
#	print(i, end=' ')

#cols = cm.jet(colimdata,1,1)

#byteimage = np.reshape(colimdata(y,x,3))
#colimage = np.reshape(colimdata,(y, x, 3))
#colimdata = colimdata[:,:,:3]

#byteimg = Image.fromarray(np.uint32(colimdata), 'L')
#print("Saving: "+ffname+"-col.png")
#byteimg.save(ffname+"-col.png")

#array_resized_image = scipy.misc.imresize(colimdata, (x, y), interp='nearest', mode=None)

#scipy.misc.imsave(ffname+"-col.png", array_resized_image)
