from os import sep
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats
from brokenaxes import brokenaxes

df = pd.read_csv('finalResults/relays.txt', sep='\t')

df['Rate'] = pd.to_numeric(round(df['Satisfied']/(df['Satisfied'] + df['TimedOut']) ,3)*100)

dfNeighbor = df[df['Scenario'] == 'neighbor']
dfRelay = df[df['Scenario'] == 'relay']

print(dfNeighbor)
print(dfRelay)

dfNeighbor = dfNeighbor.groupby(['Scenario']).agg([np.mean, np.std])
dfRelay = dfRelay.groupby(['Scenario']).agg([np.mean, np.std])

print(dfNeighbor)
print(dfRelay)

neighbor_totalInterests_mean = dfNeighbor['Interests','mean'].values.tolist()
neighbor_totalInterests_std = dfNeighbor['Interests','std'].values.tolist()
df_neighbor_totalInterests_ci = dfNeighbor['Interests','std']/np.sqrt(dfNeighbor['Interests','std'].shape[0])*1.96
neighbor_totalInterests_ci = round(df_neighbor_totalInterests_ci,3).values.tolist()

relay_totalInterests_mean = dfRelay['Interests','mean'].values.tolist()
relay_totalInterests_std = dfRelay['Interests','std'].values.tolist()
df_relay_totalInterests_ci = dfRelay['Interests','std']/np.sqrt(dfRelay['Interests','std'].shape[0])*1.96
relay_totalInterests_ci = round(df_relay_totalInterests_ci,3).values.tolist()


neighbor_rate_mean = round(dfNeighbor['Rate','mean'],2).values.tolist() #[round(num, 1) for num in dfNeighbor['Rate','mean'].values.tolist()] 
neighbor_rate_std = dfNeighbor['Rate','std'].values.tolist()
df_neighbor_rate_ci = dfNeighbor['Rate','std']/np.sqrt(dfNeighbor['Rate','std'].shape[0])*1.96
neighbor_rate_ci = round(df_neighbor_rate_ci,3).values.tolist()

relay_rate_mean = round(dfRelay['Rate','mean'],2).values.tolist() #[round(num, 1) for num in dfNeighbor['Rate','mean'].values.tolist()] 
relay_rate_std = dfRelay['Rate','std'].values.tolist()
df_relay_rate_ci = dfRelay['Rate','std']/np.sqrt(dfRelay['Rate','std'].shape[0])*1.96
relay_rate_ci = round(df_relay_rate_ci,3).values.tolist()


print(neighbor_totalInterests_mean)
print(neighbor_totalInterests_ci)
print(neighbor_rate_mean)
print(neighbor_rate_ci)

print(relay_totalInterests_mean)
print(relay_totalInterests_ci)
print(relay_rate_mean)
print(relay_rate_ci)

def autolabel(rects):
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = round(rect.get_height())
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, 0),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')


x = 1  # the label locations
width = 0.25  # the width of the bars
print(x)

#Total Interests
fig, ax = plt.subplots()
rects1 = ax.bar(x - width/2, relay_totalInterests_mean, width, color='#bf4141', label='Variante', yerr=relay_totalInterests_ci, capsize=8, ecolor='#5b5b5b')
rects2 = ax.bar(x + width/2, neighbor_totalInterests_mean, width, color='#6fa8dc', label='Original', yerr=neighbor_totalInterests_ci, capsize=5, ecolor='#5b5b5b')

ax.set_ylim(0,1800)
ax.set_xlim(.5, 1.5)
ax.set_xticks([])
# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('Interesses transmitidos')
ax.legend()

ax.yaxis.grid(linestyle='dotted')

autolabel(rects1)
autolabel(rects2)

fig.tight_layout()

plt.show()

#Satisfied Interests
fig, ax = plt.subplots()
rects1 = ax.bar(x - width/2, relay_rate_mean, width, color='#bf4141', label='Variante', yerr=relay_rate_ci, capsize=8, ecolor='#5b5b5b')
rects2 = ax.bar(x + width/2, neighbor_rate_mean, width, color='#6fa8dc', label='Original', yerr=neighbor_rate_ci, capsize=8, ecolor='#5b5b5b')

ax.set_ylim(0,100)
ax.set_xlim(.5, 1.5)
ax.set_xticks([])
# Add some text for labels, title and custom x-axis tick labels, etc.
ax.set_ylabel('% de Interesses satisfeitos')
ax.legend()

ax.yaxis.grid(linestyle='dotted')

autolabel(rects1)
autolabel(rects2)

fig.tight_layout()

plt.show()