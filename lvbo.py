#!/bin/python3

import numpy as np
import pywt
import math
from matplotlib.pyplot import plot
from matplotlib.pyplot import show
from matplotlib.pyplot import bar

a = np.loadtxt('/home/find/d/huawei/data/page_ce571.csv')

b = [int(x / 5e4) for x in a]


def sgn(num):
    if (num > 0.0):
        return 1.0
    elif (num == 0.0):
        return 0.0
    else:
        return -1.0


def all_list(arr):
    result = {}
    for i in set(arr):
        result[i] = arr.count(i)
    return result


c = all_list(b)

ckeys = sorted(c.keys())
cvals = [c[k] for k in ckeys]
jiequ = 500
ckeys1 = ckeys[:jiequ]
cvals1 = cvals[:jiequ]
# 填充以后的访问情况
d = [0 for x in range(max(ckeys1) + 1)]
for i, x in enumerate(ckeys1):
    d[x] = cvals1[i]

# for i in range(len(d) // 2):
#     avg = (d[i * 2] + d[i * 2 + 1]) / 2
#     d[i * 2] = avg
#     d[i * 2 + 1] = avg

# bar([x for x in range(len(d))], d)
# show()


db1 = pywt.Wavelet('db1')

coeffs = pywt.wavedec(d, db1, level=3)
print("------------------len of coeffs---------------------")
print(len(coeffs))
# print(coeffs)
recoeffs = pywt.waverec(coeffs, db1)
# print(recoeffs)

thcoeffs = []
for i in range(1, len(coeffs)):
    tmp = coeffs[i].copy()
    Sum = 0.0
    for j in coeffs[i]:
        Sum = Sum + abs(j)
    N = len(coeffs[i])
    Sum = (1.0 / float(N)) * Sum
    sigma = (1.0 / 0.6745) * Sum
    lamda = sigma * math.sqrt(2.0 * math.log(float(N), math.e))
    for k in range(len(tmp)):
        if (abs(tmp[k]) >= lamda):
            tmp[k] = sgn(tmp[k]) * (abs(tmp[k]) - a * lamda)
        else:
            tmp[k] = 0.0
    thcoeffs.append(tmp)
# print(thcoeffs)
usecoeffs = []
usecoeffs.append(coeffs[0])
usecoeffs.extend(thcoeffs)
# print(usecoeffs)
# recoeffs为去噪后信号
recoeffs = pywt.waverec(usecoeffs, db1)

x1 = range(len(d))
y_values = recoeffs

plot.plot(x1, y_values)
show()
