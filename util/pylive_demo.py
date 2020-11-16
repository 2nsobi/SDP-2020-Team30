from .pylive import live_plotter_xy
import numpy as np

size = 10
x_vecs = [np.zeros(size), np.zeros(size)]
y_vecs = [np.zeros(size), np.zeros(size)]
lines = []

while True:
    x_vecs[0] = np.append(x_vecs[0][1:], x_vecs[0][-1] + 1)
    x_vecs[1] = np.append(x_vecs[1][1:], x_vecs[1][-1] + 1)
    rand_val = np.random.randn(1)
    y_vecs[0] = np.append(y_vecs[0][1:], rand_val)
    y_vecs[1] = np.append(y_vecs[1][1:], rand_val)

    lines = live_plotter_xy(x_vecs, y_vecs, lines, len(y_vecs), pause_time=0.1)
