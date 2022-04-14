import numpy as np
import pandas as pd
from plotnine import *
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-s', '--strategy', choices=['multicast', 'neighbor', 'relay'], required=True)
parser.add_argument('-f', '--file', required=True)
args = parser.parse_args()

dirName = args.file.split('/')  # [0]-> directory [1]->fileName
fileName = dirName[1].split('.')[0]
print("dirName: " + dirName[0])
print("fileName: " + fileName)

relevantTypes =["OutInterests", "OutSatisfiedInterests"]    #interests sent to NetDevice, interests sent to NetDevice Satisfied

df = pd.read_csv(f'/home/osboxes/ndnSIM/simulations/{args.strategy}/{args.file}', sep='\t').dropna()
df['Kilobits'] = df['Kilobytes']*8
#remove irrelevant types
# dfCopy = df.loc[df['Type'].isin(relevantTypes)].copy()
# print(dfCopy.head())

dfNetDevice = df.loc[df['FaceDescr'].str.contains('netdev://')].copy()
dfAppFace = df.loc[df['FaceDescr'] == 'appFace://'].copy()

dfSumApp = dfAppFace.groupby(['Type']).agg({
    'Packets':'sum',
    'Kilobytes':'sum',
    'PacketRaw':'sum',
    'KilobytesRaw':'sum',
    'Kilobits':'sum'
    }).reset_index()

dfSumNetDevice = dfNetDevice.groupby(['Type']).agg({
    'Packets':'sum',
    'Kilobytes':'sum',
    'PacketRaw':'sum',
    'KilobytesRaw':'sum',
    'Kilobits':'sum'
    }).reset_index()

dfSumApp.to_csv(f'{args.strategy}/{dirName[0]}/Trace_from_Application({fileName}).txt', sep='\t', float_format='%.2f')
dfSumNetDevice.to_csv(f'{args.strategy}/{dirName[0]}/Trace_from_NetDevice({fileName}).txt', sep='\t', float_format='%.2f')

# dfRoot = dfCombined[dfCombined.Node == 'root']
# dfLeaves = dfCombined[dfCombined.Node.isin(leavesList)]

# print(dfRoot)
# print(dfLeaves)

# allNodesPlt = ggplot(dfCombined, aes(x="Time", y="Kilobits", color="Type")) + geom_point() + facet_wrap("Node") + ylab('Rate [Kbits/s]')

# rootNodePlt = ggplot(dfRoot) + geom_point(aes(x="Time", y="Kilobits", color="Type"), size=2) + geom_line(aes(x="Time", y="Kilobits", color="Type"), size=0.5) + ylab('Rate [Kbits/s]')

# allNodesPlt.draw()
# rootNodePlt.draw()

# plt.show()