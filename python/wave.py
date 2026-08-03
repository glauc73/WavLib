
from matplotlib import pyplot as plt
import numpy as np


def axes(file: str):
    x_y = []
    x_ = []
    y_ = []
    s = ""
    with open(file, "r") as dom_freq:
        s = dom_freq.read()
        x_y = s.split(',')
        for i in range(len(x_y)//2):
            x_.append(float(x_y[i].split(';')[0]))
            y_.append(float(x_y[i].split(';')[1]))
    return x_, y_




fig1, (f, f_env) = plt.subplots(1, 2)
x = []
y = []

x, y = axes("io/la_freq.txt")
f.plot(x, y)

x, y = axes("io/la_freq_env.txt")
f_env.plot(x, y)

plt.show()
