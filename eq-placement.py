import math
import os
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument("print")
args = parser.parse_args()

# x must be lesser than y
x = 2
y = 33

path = os.getcwd() + "/data/placement/"
    
gw = 1
z = 45
for k in range(x,y):
    k = k**2
    posicoes = ""
    step = math.floor(10000/math.ceil(math.sqrt(k)))
    ini = math.floor(step/2)
    gw = 1
    j = ini
    while(j<10000 and gw < k):
        i = ini
        while(i<10000 and gw < k):
            posicoes = posicoes + str(i) + " " + str(j) + " " + str(z) + "\n"
            gw+=1
            posicoes = posicoes + str(10000-i) + " " + str(10000-j) + " " + str(z) + "\n"
            gw+=1
            i+=step
        j += step
    if (k%2!=0):
        posicoes = posicoes + str(i) + " " + str(j-step) + " " + str(z) + "\n"
        gw+=1
    fileGwPlacement = path+'equidistantPlacement_'+str(gw-1)+'.dat'
    print(fileGwPlacement+': ['+str(gw-1)+' gateways]')
    if args.print == '1':
        print(posicoes)
    with open(fileGwPlacement, "w+") as outfile:
            outfile.write(posicoes)
