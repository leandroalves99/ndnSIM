from distutils import dir_util
import os
from time import time
import numpy as np
import pandas as pd
from plotnine import *
import matplotlib.pyplot as plt
import argparse
import glob
import re

parser = argparse.ArgumentParser()
parser.add_argument('-t', '--test', choices=['vehicle', 'consumer'], required=True)
args = parser.parse_args()
#Test types
#consumer =  120 vehiles  - 10C|25C|50C
#vehicle =  90V|120V|150V - 10 Consumers

dir = os.getcwd()
testScenario = '10C' if args.test == 'vehicle' else '120V'

filelist = []

for path, dirnames, filenames in os.walk(dir + '/results'):
    print(path)
    if testScenario in path:
        for file in filenames:
            if 'NetDevice' in file:
                filelist.append(os.path.join(path, file))

print(filelist)

columns = ['Test', 'Scenario', 'Interests', 'Satisfied', 'TimedOut']
dfResult = pd.DataFrame(columns=columns)
index = 0
pattern = "(.*?)V(.*?)C"

for file in filelist:
    print(file)
    dirPath = os.path.dirname(file).split(os.sep)[-2:]
    strategyName = dirPath[0]
    nVehicles = re.search(pattern, dirPath[1]).group(1)
    nConsumers = re.search(pattern, dirPath[1]).group(2)
    print(strategyName)
    print(nVehicles)
    print(nConsumers)

    df = pd.read_csv(file,'\t')
    totalInterests = df['PacketRaw'][6]
    satisfiedInterests = df['PacketRaw'][8]
    timedoutInterests = df['PacketRaw'][9]
    print(totalInterests)
    print(satisfiedInterests)
    print(timedoutInterests)
    dfResult.loc[index] = [nVehicles if args.test == 'vehicle' else nConsumers, strategyName, totalInterests, satisfiedInterests, timedoutInterests]
    index = index + 1

print(dfResult)

resultFilename = 'nVehicles.txt' if args.test == 'vehicle' else 'pConsumer.txt'

dfResult.to_csv(f'finalResults/'+resultFilename, sep='\t', index=False)

