# receiving:
# timestamp
# 2d array of {time, sensor data}
# calculate differences in time in array (x values)
# plot starting from last point
# plotting backwards, reference point is last timestamp
# subtract differences and plot that point
# subtract last local time value from first local time value
############################################

import matplotlib.pyplot as plt
from sklearn.preprocessing import normalize, MinMaxScaler
import numpy as np


############################################


def parser(board_data, timestamp):
    arr1 = board_data[0]
    arr2 = board_data[1]
    x1 = [timestamp]
    y1 = []
    x2 = [timestamp]
    y2 = []

    for i in range(len(arr1) - 1, 0, -1):
        x1.append(x1[-1] - (arr1[i][0] - arr1[i - 1][0]))
        x2.append(x2[-1] - (arr2[i][0] - arr2[i - 1][0]))

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


def plot_tcp_data(wrist_mod_data, base_mod_data, output=True):
    # wrist_mod_data and base_mod_data are both dicts of the form
    # {"adc":[array of adc readings], "loacl_ts":[array of local_ts], "beacon_ts": [array of beacon ts]}
    # the length of the 3 arrays is the same within the dict, but might be different between dicts
    if output:
        print(wrist_mod_data.keys())
        print(base_mod_data.keys())

    # remove outliers in beacon ts
    wrist_mod_data["beacon_ts"] = remove_outliers_from_beacon_ts(wrist_mod_data["beacon_ts"])
    base_mod_data["beacon_ts"] = remove_outliers_from_beacon_ts(base_mod_data["beacon_ts"])

    # transform board local_ts axis to beacon_ts axis
    wrist_x_axis = transform_axis(wrist_mod_data["local_ts"], wrist_mod_data["beacon_ts"], output=output)
    wrist_x_axis = [(x - wrist_x_axis[0]) / 1000 for x in wrist_x_axis]  # in ms
    base_x_axis = transform_axis(base_mod_data["local_ts"], wrist_mod_data["beacon_ts"], output=output)
    base_x_axis = [(x - base_x_axis[0]) / 1000 for x in base_x_axis]  # in ms

    # chop off some of adc readings to make sure y axis is the same length
    wrist_y_axis = wrist_mod_data["adc"][:len(wrist_x_axis)]
    base_y_axis = base_mod_data["adc"][:len(base_x_axis)]

    # normalize y axis
    # wrist_y_axis = normalize(np.array(wrist_y_axis).reshape(-1, 1), axis=0, norm='l1')
    # base_y_axis = normalize(np.array(base_y_axis).reshape(-1, 1), axis=0, norm='l1')
    wrist_y_scaled = MinMaxScaler()
    wrist_y = wrist_y_scaled.fit_transform(np.array(wrist_y_axis).reshape(-1, 1))

    base_y_scaled = MinMaxScaler()
    base_y = base_y_scaled.fit_transform(np.array(base_y_axis).reshape(-1, 1))

    if output:
        plt.plot(wrist_x_axis, wrist_y, label="wrist")
        plt.plot(base_x_axis, base_y, label="base")
        plt.xlabel("Beacon Timestamp in ms")
        plt.ylabel("Relative ADC readings")
        plt.legend()

        plt.show()

    return wrist_x_axis, wrist_y, base_x_axis, base_y


def remove_outliers_from_beacon_ts(beacons):
    # recieves an array of beacon timestamps, returns an array of beacon timestamps of the same length where any
    # outliers have been replaced with the beacon_ts right before it

    for i in range(1, len(beacons)):
        if beacons[i] < beacons[i - 1]:
            beacons[i] = beacons[i - 1]
        elif abs(beacons[i] - beacons[i - 1]) > 1000000:
            beacons[i] = beacons[i - 1]

    return beacons


def plot_one_board_data(data):
    # this is like the function plot_tcp_data above except it only plots one board's data
    # data is a dict of the form
    # {"adc":[array of adc readings], "loacl_ts":[array of local_ts], "beacon_ts": [array of beacon ts]}
    # the length of the 3 arrays is the same within the dict

    # transform board local_ts axis to beacon_ts axis
    x_axis = transform_axis(data["local_ts"], data["beacon_ts"])

    # chop off some of adc readings to make sure y axis is the same length
    y_axis = data["adc"][:len(x_axis)]

    # normalize y axis
    y_axis = normalize(np.array(y_axis).reshape(-1, 1), axis=0, norm='max')

    plt.plot(x_axis, y_axis, label="wrist")
    plt.xlabel("Beacon Timestamp")
    plt.ylabel("Relative ADC readings")
    plt.title("Beacon Timestamps vs Relative ADC Readings")
    plt.legend()

    plt.show()
    return


def transform_axis(local_ts, beacon_ts, output=True):
    # takes in 2 arrays of the same length, and transforms the local_ts onto the beacon_ts
    # returns an array that is the transformed local_ts onto the beacon_ts
    # ***Note*** the returned array will be slightly shorted than the length of the input arrays, specifically,
    # all repeat readings at the end of the beacon_ts array will be cut off except 1

    if len(local_ts) == 0 or len(beacon_ts) == 0:
        print("Length of array is 0, exiting")
        exit(0)

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

        # not a unique beacon
        if current_beacon_value == last_beacon_value:
            continue
        beacon_tuples.append((i, current_beacon_value))
        last_beacon_value = current_beacon_value

    end_beac_value = 0
    for k in range(1, len(beacon_tuples)):
        start_tuple = beacon_tuples[k - 1]
        end_tuple = beacon_tuples[k]
        start_beac_index, start_beac_value = start_tuple
        end_beac_index, end_beac_value = end_tuple
        if end_beac_index >= len(local_ts):
            break

        for i in range(start_beac_index, end_beac_index):
            if output:
                print(start_beac_index, end_beac_index, len(local_ts), len(beacon_ts))
            transformed_value = (local_ts[i] - local_ts[start_beac_index]) / (
                        local_ts[end_beac_index] - local_ts[start_beac_index])
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

# parser(my_data, t_stamp)

local_ts = [11358, 12566, 13470, 14470, 15470, 16420]
beacon_ts = [0, 0, 1, 1, 2, 2]
adc = [800, 800, 800, 900, 800, 800]

wrist = {"adc": adc, "local_ts": local_ts, "beacon_ts": beacon_ts}

local_ts = [1158, 1266, 1479, 1579, 1629]
beacon_ts = [0, 0, 1, 2, 2]
adc = [80, 80, 80, 95, 85]

base = {"adc": adc, "local_ts": local_ts, "beacon_ts": beacon_ts}

# plot_tcp_data(wrist, base)
