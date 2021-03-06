#!/usr/bin/env python
import sys
import numpy as np
import matplotlib.pyplot as plt
import csv
from collections import defaultdict

if len(sys.argv) < 3:
    print "Usage: viewsummary.py csvfile item"
    exit(0)

successFraction = 0.5

labelmap = {"lazy_rrgstar":"Lazy-RRG*",
            "lazy_birrgstar":"Lazy-BiRRG*",
            "prmstar":"PRM*",
            "fmmstar":"FMM*",
            "lazy_prmstar":"Lazy-PRM*",
            "lazy_rrgstar_subopt_0.1":"Lazy LBT-RRG*, eps=0.1",
            "lazy_rrgstar_subopt_0.2":"Lazy LBT-RRG*, eps=0.2",
            "rrtstar":"RRT*",
            "rrtstar_subopt_0.1":"LBT-RRT*(0.1)",
            "rrtstar_subopt_0.2":"LBT-RRT*(0.2)",
}
#labelorder = ["restart_rrt_shortcut","prmstar","fmmstar","rrtstar","birrtstar","rrtstar_subopt_0.1","rrtstar_subopt_0.2","lazy_prmstar","lazy_rrgstar","lazy_birrgstar"]
labelorder = ["prmstar","rrtstar","rrtstar_subopt_0.1","rrtstar_subopt_0.2","lazy_prmstar","lazy_rrgstar"]
dashes = [[],[8,8],[4,4],[2,2],[1,1],[12,6],[4,2,2,2],[8,2,2,2,2,2],[6,2],[2,6]]
ylabelmap = {"best cost":"Path length",
             "numEdgeChecks":"# edge checks",
}

#timevarname = 'time'
timevarname = 'numMilestones'
item = sys.argv[2]
with open(sys.argv[1],'r') as f:
    reader = csv.DictReader(f)
    items = defaultdict(list)
    for row in reader:
        time = dict()
        vmean = dict()
        vstd = dict()
        vmin = dict()
        vmax = dict()
        skip = dict()
        for (k,v) in row.iteritems():
            v = float(v) if len(v) > 0 else None
            words = k.split(None,1)
            label = words[0]
            if len(words) >= 2 and words[1] == timevarname:
                time[label] = v
        for (k,v) in row.iteritems():
            v = float(v) if len(v) > 0 else None
            words = k.split(None,1)
            label = words[0]
            if item == 'best cost' and len(words) >= 2 and words[1] == 'success fraction':
                if v < successFraction:
                    skip[label] = True
                else:
                    skip[label] = False
            if len(words) >= 2 and words[1].startswith(item):
                suffix = words[1][len(item)+1:]
                if suffix=='mean': #will have min,max,mean,etc
                    vmean[label] = v
                elif suffix=='std':
                    vstd[label] = v
                elif suffix=='max':
                    vmax[label] = v
                elif suffix=='min':
                    vmin[label] = v
                elif suffix=='':
                    vmean[label] = v
                else:
                    print "Warning, unknown suffix",suffix
        
        for label,t in time.iteritems():
            if label in skip and skip[label]:
                items[label].append((t,None))
            elif label in vmean:
                items[label].append((t,vmean[label]))
            else:
                print "Warning, no item",item,"for planner",label,"read"
    print "Available planners:",items.keys()

    fig = plt.figure(figsize=(5,3.5))
    ax1 = fig.add_subplot(111)
    if timevarname=='time':
        ax1.set_xlabel("Time (s)")
    else:
        ax1.set_xlabel("Iterations")
    ax1.set_ylabel(ylabelmap.get(item,item))
    for n,label in enumerate(labelorder):
        if label not in items: continue
        plot = items[label]
        x,y = zip(*plot)
        plannername = labelmap[label] if label in labelmap else label
        print plannername
        line = ax1.plot(x,y,label=plannername,dashes=dashes[n])
    plt.legend(loc='upper right');
    #good for bugtrap cost
    #plt.ylim([2,3])
    #good for other cost
    #plt.ylim([1,2])
    #good for edge checks
    if item=="numEdgeChecks":
        plt.ylim([0,800])
    else:
        #plt.ylim([2,2.8])
        pass
    if timevarname=='time':
        if sys.argv[1].startswith('tx90'):
            plt.xlim([0,20])
        elif sys.argv[1].startswith('baxter'):
            plt.xlim([0,60])
        elif sys.argv[1].startswith('bar_25'):
            plt.xlim([0,20])
        else:
            plt.xlim([0,10])
    else:
        plt.xlim([0,5000])

    legend = ax1.legend(loc='upper right', shadow=False)

    # The frame is matplotlib.patches.Rectangle instance surrounding the legend
    frame = legend.get_frame()
    frame.set_facecolor('0.90')

    # Set the fontsize
    for label in legend.get_texts():
        label.set_fontsize('small')
    plt.setp(ax1.get_xticklabels(),fontsize=14)
    plt.setp(ax1.get_yticklabels(),fontsize=14)

    plt.show()
