import os
from test_local_clocks import *
import matplotlib.pyplot as plt
import numpy as np
import math

def retrive_2_board_data(folder_name):
    # takes a name of a root folder and parses the two txt files of board readings
    # returns a tuple (board_1_readings, board_2_readings) where board_x_readings is a dict where the keys are the
    # beacon timestamps and the values are the local timestamps (this will get rid of duplicate readings)
    folder_path = os.path.join(os.getcwd(), "timestamp_data", folder_name)
    files = os.listdir(folder_path)
    files.remove("laptop.log")
    return parse_txt_file(os.path.join(folder_path, files[0])), parse_txt_file(os.path.join(folder_path, files[1]))


def parse_txt_file(filepath):
    # takes a path to a board txt file where the format is <beacon timestamp>,<local timestamp> on each line
    # returns a dict where the keys are the beacon timestamps and the values are the local timestamps, should get rid
    # of duplicate entries

    timestamps = {}

    f = open(filepath, 'r+')
    data = f.readlines()
    # get rid of column names
    data = data[1:]
    for line in data:
        line = line.replace("\n", "").split(",")
        beacon_ts = int(line[0])
        if beacon_ts == 0:
            continue
        local_ts = int(line[1])
        timestamps[beacon_ts] = local_ts

    f.close()
    return timestamps


def combine_board_data(board_1_dict, board_2_dict):
    # takes 2 dicts of the format {beacon_ts, local_ts} and combines them into 1 dict where the keys are the beacon
    # timestamps and the values are a dict where the key is the board number (either 1 or 2) and the value is the
    # local timestamp.
    # the combined dict will only have entries where both board_1_dict and board_2_dict have common beacon timestamps

    combined_ts = {}

    board_1_beacons = list(board_1_dict.keys())
    for beacon in board_1_beacons:
        if beacon in board_2_dict:
            combined_ts[beacon] = {1:board_1_dict[beacon], 2:board_2_dict[beacon]}
    return combined_ts


def total_drift(combined_ts, verbose = True, graph_drift = True, graph_elapsed = True, limit = None):
    # takes a combined dict from combine_board_data and gets the total drift between the local clocks over the
    # entire experiment.  The drift is measured by abs((clock1_end - clock1_start) - (clock2_end - clock2_start))
    # the time of the experiment is measured by lastbeacon - firstbeacon
    beacons = list(combined_ts.keys())
    beacons.sort()
    if limit is not None:
        beacons = beacons[:limit]
    first_beacon = beacons[0]
    last_beacon = beacons[len(beacons) -1]
    experiment_time_ms = (last_beacon - first_beacon)/(1000 * 1.024) # in ms
    experiment_time_s = experiment_time_ms/1000
    experiment_time_min = experiment_time_s/60
    if verbose:
        print("Experiment time in ms:\t{}".format(experiment_time_ms))
        print("Experiment time in s:\t{}".format(experiment_time_s))
        print("Experiment time in min:\t{}".format(experiment_time_min))

    clock1_end = combined_ts[last_beacon][1]
    clock1_start = combined_ts[first_beacon][1]
    clock2_end = combined_ts[last_beacon][2]
    clock2_start = combined_ts[first_beacon][2]

    local_drift_ms = abs((clock1_end - clock1_start) - (clock2_end - clock2_start))
    if verbose:
        print("Local drift in ms:\t{}".format(local_drift_ms))

    clock1_elapsed = [combined_ts[x][1] - clock1_start for x in beacons]
    clock2_elapsed = [combined_ts[x][2] - clock2_start for x in beacons]
    drift_over_time = [c1 - c2 for c1, c2 in zip(clock1_elapsed, clock2_elapsed)]
    beacons_s = [(i - first_beacon) / (1000000 * 1.024) for i in beacons]  # in seconds

    if verbose:
        max_drift = abs(max(drift_over_time))
        min_drift = abs(min(drift_over_time))
        overall_max = max(max_drift, min_drift)
        print("Max drift:\t{}".format(overall_max))

    if graph_elapsed:
        plt.plot(beacons_s, clock1_elapsed, 'r', label = "clock1")
        plt.plot(beacons_s, clock2_elapsed, 'b', label = "clock2")
        plt.legend()
        plt.show()

    if graph_drift:
        if verbose:
            print("x axis:")
            print(beacons_s[:50])
            print("y_axis")
            print(drift_over_time[:50])
        plt.plot(beacons_s, drift_over_time)
        plt.xlabel("Beacon time (s)")
        plt.ylabel("Drift between the clocks (ms)")
        plt.title("Drift between 2 CC3220SF Local Clocks over a {:4f}-minute experiment".format(experiment_time_min))
        plt.show()


    return experiment_time_s, local_drift_ms


def drift_over_x_beacons(combined_ts, num_beacons = None, absolute = True):
    # drift over 1 beacon interval, only account for successive beacons (around 100ms between them)
    beacons = list(combined_ts.keys())
    beacons.sort()

    # drift between clock1 and clock2 over 1 beacon
    drift_between = []

    for i in range(len(beacons)):
        if i == 0:
            continue
        beac_interval_ms = (beacons[i] - beacons[i-1])/(1000 * 1.024) # in ms
        if beac_interval_ms < 150:
            clock1_start = combined_ts[beacons[i-1]][1]
            clock1_end = combined_ts[beacons[i]][1]
            clock1_elapsed_ms = clock1_end - clock1_start

            clock2_start = combined_ts[beacons[i-1]][2]
            clock2_end = combined_ts[beacons[i]][2]
            clock2_elapsed_ms = clock2_end - clock2_start

            if absolute:
                drift = abs(clock1_elapsed_ms - clock2_elapsed_ms)
            else:
                drift = clock1_elapsed_ms - clock2_elapsed_ms
            drift_between.append(drift)
    trials = len(drift_between)

    # calculate avg and conf interval
    np_drift = np.array(drift_between)
    avg = np.mean(np_drift)
    conf = 2 * math.sqrt(np.var(drift_between)/trials)
    print("Avg drift over 1 beacon interval:\n{}\n95% Confidence Interval:\n{}, {}".format(avg, avg-conf, avg+conf))

    drift_between.sort()
    ninety_nine = int((len(drift_between) * 0.99) -1)
    print("99% of drift is less than {}".format(drift_between[ninety_nine]))
    ninety = int((len(drift_between) * 0.9) - 1)
    print("90% of drift is less than {}".format(drift_between[ninety]))

    plt.hist(drift_between, bins=30)
    plt.title("Drift between two CC3220SF local clocks over 1 100ms beacon interval,\n{} trials".format(trials))
    plt.xlabel("Drift in ms")
    plt.ylabel("frequency")
    plt.show()


def recover_experiment_from_log(folder_name):
    #used to recreate the board txt files from the laptop log file in the case that the board txt files get corrupted
    folder_path = os.path.join(os.getcwd(), "timestamp_data", folder_name)
    log_file = os.path.join(folder_path, "laptop.log")
    if not os.path.exists(log_file):
        print("no log file, exiting")
        exit(-1)
    f = open(log_file, 'r+')
    data = f.readlines()
    #data = data[:1000]
    connect_str = "*** connection from ('"
    next_line_is_data = False
    ip_addr = ""
    for line in data:
        if next_line_is_data:
            parse_mcu_msg(line, (ip_addr, 0), store_folder=folder_name)
            next_line_is_data = False
        if line.find(connect_str)>=0:
            ip_addr = line[len(connect_str):].split("'")[0]
            next_line_is_data = True

    f.close()



if __name__ == '__main__':
    folder_name = "2_boards_2021-03-23_14;33;16"
    a, b = retrive_2_board_data(folder_name)
    c = combine_board_data(a, b)
    drift_over_x_beacons(c)
    total_drift(c, graph_elapsed=True)
    #recover_experiment_from_log(folder_name)