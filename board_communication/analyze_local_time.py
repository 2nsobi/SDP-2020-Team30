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

    for f in files:
        if f.find(".log")>=0:
            files.remove(f)

    return parse_txt_file(os.path.join(folder_path, files[0])), parse_txt_file(os.path.join(folder_path, files[1]))


def parse_txt_file(filepath, beacon_test = False, array = False):
    # takes a path to a board txt file where the format is <beacon timestamp>,<local timestamp> on each line
    # returns a dict where the keys are the beacon timestamps and the values are the local timestamps, should get rid
    # of duplicate entries
    # if beacon test is true, returns an array of the beacon timestamps
    # if array is true, instead returns a 2d array of the text file data where the 1st dimension is each line and the
    # 2nd dimension is the elements in the line separated by ","

    timestamps = {}
    beacons = []
    arr = []

    f = open(filepath, 'r+')
    data = f.readlines()
    # get rid of column names
    data = data[1:]
    for line in data:
        line = line.replace("\n", "").split(",")
        if array:
            arr.append([int(x) for x in line])
            continue
        beacon_ts = int(line[0])
        beacons.append(beacon_ts)
        if beacon_ts == 0 or beacon_test:
            continue
        local_ts = int(line[1])
        timestamps[beacon_ts] = local_ts

    f.close()
    if beacon_test:
        return beacons
    if array:
        return arr
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
    # takes a combined dict from combine_board_data
    # (dict where the keys are the beacon
    #     # timestamps and the values are a dict where the key is the board number (either 1 or 2) and the value is the
    #     # local timestamp. )

    # and gets the total drift between the local clocks over the
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

    clock1_elapsed = [(combined_ts[x][1] - clock1_start) for x in beacons] # in ms
    clock2_elapsed = [(combined_ts[x][2] - clock2_start) for x in beacons] # in ms
    drift_over_time = [c1 - c2 for c1, c2 in zip(clock1_elapsed, clock2_elapsed)]
    print("Clock1 elapsed (ms)")
    print(clock1_elapsed[:50])
    print("Clock2 elapsed (ms)")
    print(clock2_elapsed[:50])
    print("Drift between elapsed:")
    print(drift_over_time[:50])
    beacons_s = [(i - first_beacon) / (1000000 * 1.024) for i in beacons]  # in seconds

    if verbose:
        max_drift = abs(max(drift_over_time))
        min_drift = abs(min(drift_over_time))
        overall_max = max(max_drift, min_drift)
        print("Max drift:\t{}".format(overall_max))

    if graph_elapsed:
        plt.plot(beacons_s, clock1_elapsed, 'r', label = "clock1")
        plt.plot(beacons_s, clock2_elapsed, 'b', label = "clock2")
        plt.xlabel("Elapsed time in seconds based on beacon timestamps")
        plt.ylabel("Elapsed time in seconds on local clocks")
        plt.title("Beacon Timestamps vs Local Clock readings over {} minutes".format(experiment_time_min))
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

        # graph drift again but take out jumps in elapsed time more than 150ms
        new_drift = []
        drift_beacs = []
        for i in range(1, len(beacons)):
            c1_int = clock1_elapsed[i] - clock1_elapsed[i-1]
            c2_int = clock2_elapsed[i] - clock2_elapsed[i-1]
            if c1_int < 150 and c2_int <150:
                new_drift.append(c1_int - c2_int)
                drift_beacs.append(beacons[i-1])
        plt.plot(drift_beacs, new_drift)
        plt.xlabel("Beacon time (s)")
        plt.ylabel("Drift between the clocks (ms)")
        plt.title("Drift between 2 Local Clocks excluding periods longer than 150ms\n"
                  "over a {:4f}-minute experiment".format(experiment_time_min))
        plt.show()

    # graph delta t (over 1 beacon) over the experiment

    #get intervals
    clock_1_intervals = []
    clock_2_intervals = []
    int_beacons = []
    for i in range(1, len(beacons)):
        c1_int = combined_ts[beacons[i]][1] - combined_ts[beacons[i-1]][1]
        c2_int = combined_ts[beacons[i]][2] - combined_ts[beacons[i - 1]][2]
        if c1_int < 200 and c2_int < 200:
            clock_1_intervals.append(c1_int)
            clock_2_intervals.append(c2_int)
            int_beacons.append(beacons[i-1])
    plt.plot(int_beacons, clock_1_intervals, 'r', label="clock1")
    plt.plot(int_beacons, clock_2_intervals, 'b', label="clock2")
    plt.xlabel("Elapsed time in seconds based on beacon timestamps")
    plt.ylabel("Elapsed time in seconds on local clocks since last beacon")
    plt.title("Beacon Timestamps vs Local Clock interval over {} minutes".format(experiment_time_min))
    plt.legend()
    plt.show()

    plt.hist(clock_1_intervals, bins=20)
    plt.title("Elapsed time on one clock between beacons")
    plt.ylabel("Frequency")
    plt.xlabel("Elapsed time on local clock between 102.4ms beacons (ms)")
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
    if absolute:
        plt.title("Absolute Value of Drift Between Two Local Clocks \nover 102.4ms Beacon Intervals, {} trials".format(trials))
    else:
        plt.title("Relative Drift Between Two Local Clocks \nover 102.4ms Beacon Intervals, {} trials".format(trials))
    plt.xlabel("Drift (ms)")
    plt.ylabel("Frequency")
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


def analyze_beacons(beacons):
    #beacons is an array of beacon timestamps
    num_beacons = len(beacons)
    missed = 0
    two_in_a_row = 0
    for i in range(1, num_beacons):
        diff = beacons[i] - beacons[i-1]

        # if diff was greater than 1 second, faulty beacon timestamp
        if diff>1000000:
            continue
        if diff > 150000:
            missed +=1
        if diff > 270000:
            two_in_a_row +=1
            print(beacons[i], beacons[i-1])
    print("{}/{} beacons missed, {:4f}% of beacons recieved".format(missed, num_beacons,
                                                                    100*(num_beacons-missed)/num_beacons))
    print("Out of {} beacons, there were {} instances of missing 2 in a row".format(num_beacons,
                                                                    two_in_a_row))


def local_clock_test(folder_name):
    folder_name = "2_boards_2021-03-23_14;33;16"
    #folder_name = "2021-03-24_22;32;48"
    a, b = retrive_2_board_data(folder_name)
    c = combine_board_data(a, b)
    total_drift(c, graph_elapsed=True)
    drift_over_x_beacons(c)
    drift_over_x_beacons(c, absolute=False)
    # recover_experiment_from_log(folder_name)

def beacon_recv_percent(folder, hw = False):
    # if using a file from the beacon_recv_test folder, <folder> should be the name of the subfolder and hw should be
    # false.  If using a file from the hw_timestamp_data folder, <folder> should be the name of the file in
    # hw_timestamp_data and hw should be true
    if not hw:
        folder_path = os.path.join(os.getcwd(), "beacon_recv_test", folder)
        files = os.listdir(folder_path)
        for f in files:
            if f.find(".log") >=0:
                files.remove(f)
        txt_file = files[0]
        txt_file = os.path.join(folder_path, txt_file)
    else:
        txt_file = os.path.join(os.getcwd(), "hw_timestamp_data", folder)
    a = parse_txt_file(txt_file, beacon_test=True)
    analyze_beacons(a)


def analyze_hw_ts(filename):
    # filename is a txt file in the hw_timestamp_data folder where each line is (beacon_ts, hw_ts, local_ts)
    txt_file = os.path.join(os.getcwd(), "hw_timestamp_data", filename)

    # 2d array where first dimension is each line and second dimension is (beacon_ts, hw_ts, local_ts) for that line
    data = parse_txt_file(txt_file, array=True)

    # array of hw timestamps in ms
    hw_ts = [x[1]/1000 for x in data]

    # array of local timestamps
    loc_ts = [x[2] for x in data]


    # get rid of any extremely large intervals (>200ms) or small (<50ms) for missed beacons and create intervals array
    hw_intervals = []
    loc_intervals = []
    for i in range(1, len(hw_ts)):
        hw_interval = hw_ts[i] - hw_ts[i-1]# in ms
        loc_interval = loc_ts[i] - loc_ts[i-1] # in ms
        if hw_interval > 200 or hw_interval < 50:
            continue
        hw_intervals.append(hw_interval)
        loc_intervals.append(loc_interval)
    trials = len(hw_intervals)
    print(hw_intervals)
    print(max(hw_intervals))
    print(min(hw_intervals))
    #plot a histogram of intervals between hw_timestamps (gives an idea of how consistently the beacons arrive)
    plt.hist(hw_intervals, bins=30)
    plt.title("Intervals between hardware timestamps,\n{} trials".format(trials))
    plt.xlabel("Interval in ms")
    plt.ylabel("frequency")
    plt.show()

    # plot histogram of local_ts (gives an idea of the variability of local_ts readings)
    plt.hist(loc_intervals, bins=30)
    plt.title("Intervals between local timestamps,\n{} trials".format(trials))
    plt.xlabel("Interval in ms")
    plt.ylabel("frequency")
    plt.show()




if __name__ == '__main__':
    #folder_name = "2021-03-29_11;36;06.txt"
    #folder_name = "2021-03-29_12;53;26.txt"
    #beacon_recv_percent(folder_name, True)
    #folder_name = "2021-03-25_19;39;18"
    #beacon_recv_percent(folder_name)
    #analyze_hw_ts(folder_name)
    folder_name = "2_boards_2021-03-23_14;33;16"
    local_clock_test(folder_name)