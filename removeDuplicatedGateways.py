import pandas as pd
from glob import glob

path = '/home/rogerio/git/sim-res/datafile/optimized-oriented/input_data/100x1/qos_b0.9/optimizedPlacement*.dat'

files = sorted(glob(path))

for file in files:
    data = pd.read_csv(file, names=['x', 'y', 'z'], sep=" ", index_col=False)
    data.drop_duplicates(inplace=True)
    # print(len(data))
    newfilename = file[0:71] + "optGPlacement_" + file[99:]
    print(newfilename + " has " + str(len(data)) + " gateways")
    data.to_csv(newfilename, header=False, sep=" ", index=False)
    # print(newfilename + " SAVED!")
print(len(files))
