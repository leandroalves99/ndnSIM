from os import sep
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats
from brokenaxes import brokenaxes
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-f', '--file', required=True)
args = parser.parse_args()

df = pd.read_csv(f'finalResults/{args.file}', sep='\t')

labels = df['Test'].unique().tolist()
print(labels)
df['Rate'] = pd.to_numeric(round(df['Satisfied']/(df['Satisfied'] + df['TimedOut']) ,3)*100)

dfNeighbor = df[df['Scenario'] == 'neighbor']
dfMulticast = df[df['Scenario'] == 'multicast']
print(dfNeighbor['Rate'])

dfNeighbor = dfNeighbor.groupby(['Test','Scenario']).agg([np.mean, np.std])
dfMulticast = dfMulticast.groupby(['Test','Scenario']).agg([np.mean, np.std])

print(dfNeighbor)
print(dfMulticast)

neighbor_totalInterests_mean = dfNeighbor['Interests','mean'].values.tolist()
neighbor_totalInterests_std = dfNeighbor['Interests','std'].values.tolist()
df_neighbor_totalInterests_ci = dfNeighbor['Interests','std']/np.sqrt(dfNeighbor['Interests','std'].shape[0])*1.96
neighbor_totalInterests_ci = round(df_neighbor_totalInterests_ci,3).values.tolist()

multicast_totalInterests_mean = dfMulticast['Interests','mean'].values.tolist()
multicast_totalInterests_std = dfMulticast['Interests','std'].values.tolist()
df_multicast_totalInterests_ci = dfMulticast['Interests','std']/np.sqrt(dfMulticast['Interests','std'].shape[0])*1.96
multicast_totalInterests_ci = round(df_multicast_totalInterests_ci,3).values.tolist()

neighbor_rate_mean = round(dfNeighbor['Rate','mean'],2).values.tolist() #[round(num, 1) for num in dfNeighbor['Rate','mean'].values.tolist()] 
neighbor_rate_std = dfNeighbor['Rate','std'].values.tolist()
df_neighbor_rate_ci = dfNeighbor['Rate','std']/np.sqrt(dfNeighbor['Rate','std'].shape[0])*1.96
neighbor_rate_ci = round(df_neighbor_rate_ci,3).values.tolist()

multicast_rate_mean = dfMulticast['Rate','mean'].values.tolist()
multicast_rate_std = dfMulticast['Rate','std'].values.tolist()
df_multicast_rate_ci = dfMulticast['Rate','std']/np.sqrt(dfMulticast['Rate','std'].shape[0])*1.96
multicast_rate_ci = round(df_multicast_rate_ci,3).values.tolist()

print(neighbor_totalInterests_mean)
print(neighbor_totalInterests_ci)
print(neighbor_rate_mean)
print(neighbor_rate_ci)
print(multicast_totalInterests_mean)
print(multicast_totalInterests_ci)
print(multicast_rate_mean)
print(multicast_rate_ci)


def autolabel(rects):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = round(rect.get_height())
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, 0),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')


x = np.arange(len(labels))  # the label locations
print(x)
width = 0.25  # the width of the bars
 
fig, (ax,ax2) = plt.subplots(2,1,sharex=True)


#Total interests
rects1 = ax.bar(x - width/2, neighbor_totalInterests_mean, width, color='#6fa8dc', label='Neighbor', yerr=neighbor_totalInterests_ci, capsize=5, ecolor='#5b5b5b')
rects2 = ax.bar(x + width/2, multicast_totalInterests_mean, width, color='#93c47d', label='Multicast', yerr=multicast_totalInterests_ci, capsize=5, ecolor='#5b5b5b')
rects3 = ax2.bar(x - width/2, neighbor_totalInterests_mean, width, color='#6fa8dc', label='Neighbor', yerr=neighbor_totalInterests_ci, capsize=5, ecolor='#5b5b5b')
rects4 = ax2.bar(x + width/2, multicast_totalInterests_mean, width, color='#93c47d', label='Multicast', yerr=multicast_totalInterests_ci, capsize=5, ecolor='#5b5b5b')


# zoom-in / limit the view to different portions of the data
ax.set_ylim(10000, 100000)  # outliers only   (10000, 100000) 
ax2.set_ylim(0, 5000)  # most of the data       (0, 2000)   (0, 5000) 

# hide the spines between ax and ax2
ax.spines['bottom'].set_visible(False)
ax2.spines['top'].set_visible(False)
ax.xaxis.tick_top()
ax.tick_params(labeltop=False)  # don't put tick labels at the top
ax2.xaxis.tick_bottom()

d = .010  # how big to make the diagonal lines in axes coordinates
# arguments to pass to plot, just so we don't keep repeating them
kwargs = dict(transform=ax.transAxes, color='k', clip_on=False)
ax.plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
ax.plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

kwargs.update(transform=ax2.transAxes)  # switch to the bottom axes
ax2.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
ax2.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal

# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Interesses transmitidos')
ax.yaxis.set_label_coords(-.1, 0)
ax2.set_xlabel('Número de Veículos' if args.file == 'nVehicles.txt' else '% de Consumidores')
ax2.set_xticks(x)
ax2.set_xticklabels(labels)
ax.legend()

ax.yaxis.grid(linestyle='dotted')
ax2.yaxis.grid(linestyle='dotted')

autolabel(rects2)
autolabel(rects3)

fig.tight_layout()

plt.show()

#Satisfied Interests
fig, ax = plt.subplots()
rects1 = ax.bar(x - width/2, neighbor_rate_mean, width, color='#6fa8dc', label='Neighbor', yerr=neighbor_rate_ci, capsize=5, ecolor='#5b5b5b')
rects2 = ax.bar(x + width/2, multicast_rate_mean, width, color='#93c47d', label='Multicast', yerr=multicast_rate_ci, capsize=5, ecolor='#5b5b5b')

ax.set_ylim(0,100)
# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('% de Interesses satisfeitos')
ax.set_xlabel('Número de Veículos' if args.file == 'nVehicles.txt' else '% de Consumidores')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.legend()

ax.yaxis.grid(linestyle='dotted')

autolabel(rects1)
autolabel(rects2)

fig.tight_layout()

plt.show()