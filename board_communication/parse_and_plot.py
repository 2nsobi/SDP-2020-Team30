# receiving:
# timestamp
# 2d array of {time, sensor data}
# calculate differences in time in array (x values)
# plot starting from last point
# plotting backwards, reference point is last timestamp
# subtract differences and plot that point
# subtract last local time value from first local time value
############################################

import  matplotlib.pyplot as plt
from sklearn.preprocessing import normalize
import numpy as np

############################################


def parser(board_data, timestamp):

    arr1 = board_data[0]
    arr2 = board_data[1]
    x1 = [timestamp]
    y1 = []
    x2 = [timestamp]
    y2 = []

    for i in range(len(arr1)-1, 0, -1):
        x1.append(x1[-1] - (arr1[i][0] - arr1[i-1][0]))
        x2.append(x2[-1] - (arr2[i][0] - arr2[i-1][0]))

    for j in range(0, len(arr1)):
        y1.append(arr1[j][1])
        y2.append(arr2[j][1])

    x1 = x1[::-1]
    x2 = x2[::-1]

    print(x1)
    print(x2)
    print(y1)
    print(y2)

    plt.scatter(x1, y1)
    plt.scatter(x2, y2)
    plt.show()

def plot_tcp_data(wrist_mod_data, base_mod_data):
    # wrist_mod_data and base_mod_data are both dicts of the form
    # {"adc":[array of adc readings], "loacl_ts":[array of local_ts], "beacon_ts": [array of beacon ts]}
    # the length of the 3 arrays is the same within the dict, but might be different between dicts

    # transform board local_ts axis to beacon_ts axis
    wrist_x_axis = transform_axis(wrist_mod_data["local_ts"], wrist_mod_data["beacon_ts"])
    base_x_axis = transform_axis(base_mod_data["local_ts"], wrist_mod_data["beacon_ts"])

    # chop off some of adc readings to make sure y axis is the same length
    wrist_y_axis = wrist_mod_data["adc"][:len(wrist_x_axis)]
    base_y_axis = base_mod_data["adc"][:len(base_x_axis)]

    # normalize y axis
    wrist_y_axis = normalize(np.array(wrist_y_axis).reshape(-1, 1), axis=0, norm='max')
    base_y_axis = normalize(np.array(base_y_axis).reshape(-1, 1), axis=0, norm='max')


    plt.plot(wrist_x_axis, wrist_y_axis, label="wrist")
    plt.plot(base_x_axis, base_y_axis, label="base")
    plt.xlabel("Beacon Timestamp")
    plt.ylabel("Relative ADC readings")
    plt.legend()

    plt.show()


    return 1

def transform_axis(local_ts, beacon_ts):
    # takes in 2 arrays of the same length, and transforms the local_ts onto the beacon_ts
    # returns an array that is the transformed local_ts onto the beacon_ts
    # ***Note*** some of the end of the local

    # make sure inputs are floats
    local_ts = [float(x) for x in local_ts]
    beacon_ts = [float(x) for x in beacon_ts]

    # array to return
    transformed_x_axis = []

    # array of tuples (index of unique beacon, value of unique beacon)
    beacon_tuples = []

    last_beacon_value = -1
    for i in range(len(beacon_ts)):
        current_beacon_value = beacon_ts[i]

        #not a unique beacon
        if current_beacon_value == last_beacon_value:
            continue
        beacon_tuples.append((i, current_beacon_value))
        last_beacon_value = current_beacon_value



    for k in range(1, len(beacon_tuples)):
        start_tuple = beacon_tuples[k-1]
        end_tuple = beacon_tuples[k]
        start_beac_index, start_beac_value = start_tuple
        end_beac_index, end_beac_value = end_tuple

        for i in range(start_beac_index, end_beac_index):
            transformed_value = (local_ts[i] - local_ts[start_beac_index])/(local_ts[end_beac_index] - local_ts[start_beac_index])
            transformed_value *= (end_beac_value - start_beac_value)
            transformed_value += start_beac_value
            transformed_x_axis.append(transformed_value)

    transformed_x_axis.append(end_beac_value)

    return transformed_x_axis


# timestamp = 100
array1 = [(1, 1), (3, 2), (6, 3), (10, 7)]
array2 = [(3, 2), (4, 3), (5, 4), (9, 14)]

my_data = [array1, array2]
t_stamp = 100

#parser(my_data, t_stamp)

local_ts = [11358, 12566, 13470, 14470, 15470, 16420]
beacon_ts = [0, 0, 1, 1, 2, 2]
adc = [800, 800, 800, 900, 800, 800]

wrist = {"adc": adc, "local_ts" : local_ts, "beacon_ts": beacon_ts}

local_ts = [1158, 1266, 1479, 1579, 1629]
beacon_ts = [0, 0, 1, 2, 2]
adc = [80, 80, 80, 95, 85]

base = {"adc": adc, "local_ts" : local_ts, "beacon_ts": beacon_ts}

plot_tcp_data(wrist, base)
