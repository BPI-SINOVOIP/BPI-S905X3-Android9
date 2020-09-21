import numpy as np
import matplotlib.pyplot as plt
import collections

HZ = 500;
T_SPAN = 10;
T_avg = 10;
plt.ion() # This allows for interactive plots, i.e. plots that do not block execution of the script

wave = collections.deque(np.zeros(HZ*T_SPAN), maxlen=HZ * T_SPAN)
buff = collections.deque(np.zeros(HZ*T_SPAN), maxlen=HZ * T_avg)

while True:
	for i in xrange(HZ):
		buff.append(float(raw_input()))		
		wave.append(sum(buff)/float(len(buff)))
	plt.clf()
	plt.plot(range(HZ * T_SPAN), list(wave))
	plt.axis([-1, T_SPAN*HZ, 0.0, 0.4])
	plt.pause(0.01)
