from matplotlib import pyplot as plt
import numpy as np
with open('tracks/chama.wav', 'rb') as f:
    sound = f.read()

sample_rate = int.from_bytes(sound[24:28], byteorder='little')
num_chanels = int.from_bytes(sound[22:24], byteorder='little')
bytes_per_sample = int.from_bytes(sound[34:36], byteorder='little')//8
len = int.from_bytes(sound[40:44], byteorder='little')
byte_rate = int.from_bytes(sound[28:32], byteorder='little')
time_duration = len/byte_rate

num_samples = len // (bytes_per_sample)
print(round(time_duration,2))

_x = []
_y = []
for i in range(num_samples):
    inicio = 44 + i*bytes_per_sample
    fim = 44 + (i + 1)*bytes_per_sample
    sample = int.from_bytes(sound[inicio : fim], byteorder='little', signed=True)

    _x.append(i/sample_rate)
    _y.append(sample)

x = np.array(_x)*343
y = np.array(_y)

xticks_dict = {i*sample_rate : str(i) for i in range(int(np.ceil(time_duration)))}

fig, ax = plt.subplots()
ax.set(xlabel='time (s)', ylabel='Amplitude')
ax.grid()
ax.plot(x, y)

plt.show()
fig.savefig('images/chama.png')

