import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from glob import glob

path = '/home/rogerio/git/sim-res/datafile/optimized-oriented/input_data/optimizedPlacement*.dat'
path2 = '/home/rogerio/git/sim-res/datafile/optimized-oriented/input_data/optimizedPlacement_1s_100x1Gv_100D.dat'
files = sorted(glob(path))

# for file in files:
data = pd.read_csv(path2, names=['x', 'y', 'z'], sep=" ", index_col=False)
data.drop_duplicates(inplace=True)
newfilename = path2[0:84] + "optGPlacement_" + path2[103:]
data.to_csv(newfilename, header=False, sep=" ", index=False)
print(newfilename + " SAVED!")
