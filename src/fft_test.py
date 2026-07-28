
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




fig1, (ax_tb, ax_ta, ax_tacov) = plt.subplots(1, 3)
fig2, (ax_fb, ax_fa) = plt.subplots(1, 2)
x = []
y = []

x, y = axes("io/dom_time_before.txt")
ax_tb.plot(x, y)

x, y = axes("io/dom_time_after.txt")
ax_ta.plot(x, y)

x, y = axes("io/dom_time_after_covolution.txt")
ax_tacov.plot(x, y)

ax_ta.set_xlabel("time")
ax_tb.set_ylabel("Amplitude")
ax_tb.set_xlabel("time")

ax_tb.set_title("Before the filter")
ax_ta.set_title("After the filter")

fig1.savefig("images/time_domain.png")

x, y = axes("io/dom_freq_before.txt")
ax_fb.plot(x, y)

x, y = axes("io/dom_freq_after.txt")
ax_fa.plot(x, y)
ax_fa.set_xlabel("frequency")
ax_fb.set_ylabel("|Amplitude|")
ax_fb.set_xlabel("frequency")

ax_fb.set_title("Before the filter")
ax_fa.set_title("After the filter")


fig2.savefig("images/freq_domain.png")
plt.show()
