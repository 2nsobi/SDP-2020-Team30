import os
import matplotlib.pyplot as plt
import numpy


def parse_same_time_experiment(experiment_folder, plot = True):
    wrist_file = os.path.join(experiment_folder, "wrist_data.txt")
    base_file = os.path.join(experiment_folder, "base_data.txt")

    f = open(wrist_file, 'r')
    wrist_x = f.readline()
    wrist_x = wrist_x.split(",")
    wrist_x = [float(x) for x in wrist_x]
    wrist_y = f.readline()
    wrist_y = wrist_y.replace("[", "").replace("]", "").replace(" ", "")
    wrist_y = wrist_y.split(",")
    wrist_y = [float(x) for x in wrist_y]
    f.close()

    f = open(base_file, 'r')
    base_x = f.readline()
    base_x = base_x.split(",")
    base_x = [float(x) for x in base_x]
    base_y = f.readline()
    base_y = base_y.replace("[", "").replace("]", "").replace(" ", "")
    base_y = base_y.split(",")
    base_y = [float(x) for x in base_y]
    f.close()

    wrist_derivative = numpy.gradient(wrist_y)
    base_derivative = numpy.gradient(base_y)

    if plot:
        fig, ax = plt.subplots(2)
        ax[0].plot(wrist_x, wrist_y, label="wrist")
        ax[0].plot(base_x, base_y, label="base")

        ax[0].set_xlabel("Beacon Timestamp in ms")
        ax[0].set_ylabel("Relative ADC readings")
        ax[0].legend()

        ax[1].plot(wrist_x, wrist_derivative, label="wrist_derivative")
        ax[1].plot(base_x, base_derivative, label="base_derivative")

        ax[1].set_xlabel("Beacon Timestamp in ms")
        ax[1].set_ylabel("Derivative of ADC Readings")
        ax[1].legend()
        plt.show()

    return wrist_x, wrist_y, wrist_derivative, base_x, base_y, base_derivative


def calculate_timestamps(wrist_x, wrist_der, base_x, base_der, threshold = 0.1):
    # uses the derivative of base and wrist to timestamp data
    # timestamp is first time derivative exceeds threshold

    wrist_ts = None
    base_ts = None

    for i in range(len(wrist_der)):
        if abs(wrist_der[i]) > threshold:
            wrist_ts = wrist_x[i]
            break

    for i in range(len(base_der)):
        if abs(base_der[i]) > threshold:
            base_ts = base_x[i]
            break
    return wrist_ts, base_ts

def plot_with_ts(wrst_x, wrst_y, wrst_der, wrst_ts, base_x, base_y, base_der, base_ts, folder):
    fig, ax = plt.subplots(2)
    ax[0].plot(wrst_x, wrst_y, label="wrist")
    ax[0].plot(base_x, base_y, label="base")

    ax[0].set_xlabel("Beacon Timestamp in ms")
    ax[0].set_ylabel("Relative ADC readings")

    ax[1].plot(wrst_x, wrst_der, label="wrist_derivative")
    ax[1].plot(base_x, base_der, label="base_derivative")

    ax[1].set_xlabel("Beacon Timestamp in ms")
    ax[1].set_ylabel("Derivative of ADC Readings")
    ax[0].vlines(wrst_ts, ymin=0, ymax= 1, linestyles='dashed', label='wrist_ts')
    ax[0].vlines(base_ts, ymin=0, ymax=1, linestyles='dashed', label='base_ts')
    ax[1].vlines(wrst_ts, ymin=0, ymax=1, linestyles='dashed', label='wrist_ts')
    ax[1].vlines(base_ts, ymin=0, ymax=1, linestyles='dashed', label='base_ts')
    ax[1].legend()
    ax[0].legend()
    plt.suptitle("Wrist_ts: {}, Base_ts: {}, diff: {}\n{}".format(wrst_ts,
                                                                  base_ts,
                                                                  wrst_ts-base_ts,
                                                                  folder))
    plt.show()


def same_time_experiment_reconstruct(folder = "2021-04-15_23;29;40"):
    cwd = os.getcwd()
    cwd = os.path.split(cwd)[0]
    experiment_folder = os.path.join(cwd, "same_time_trigger_data", folder)
    parse_same_time_experiment(experiment_folder)

def get_all_timestamps(folder = "same_time", hist = True, plot_greater_than = None, hist_limit = None):
    # folder is the root folder containing one folder for each experiment, each experiment folder contains
    # a wrist_data.txt and base_data.txt file
    # if plot_greater than is not none, it should be an int, and for each experiment where the difference is
    # greater than <plot_greater_than> the experiment will be plotted
    # if hist_limit is not none, then it should be an integer that will limit the x axis range for the histogram
    if folder is "same_time":
        folder = os.path.join(os.getcwd(), "same_time_trigger_data")
    if folder is "slomo":
        folder = os.path.join(os.getcwd(), "slo_mo_testing")

    #print(len(os.listdir(folder)))

    diff = []

    for experiment in os.listdir(folder):
        experiment_folder = os.path.join(folder, experiment)
        wrst_x, wrst_y, wrst_der, base_x, base_y, base_der = parse_same_time_experiment(experiment_folder,
                                                                                        plot=False)
        wrist_ts, base_ts = calculate_timestamps(wrst_x, wrst_der,base_x, base_der)
        exp_diff = wrist_ts - base_ts
        if plot_greater_than is not None:
            if abs(exp_diff) > plot_greater_than:
                plot_with_ts(wrst_x, wrst_y, wrst_der, wrist_ts,
                             base_x, base_y, base_der, base_ts, experiment_folder)
        if hist_limit is not None:
            if abs(exp_diff) < hist_limit:
                diff.append(exp_diff)
        else:
            diff.append(exp_diff)
        print(wrist_ts, base_ts, diff)

    if hist:
        plt.hist(diff, bins=30)
        plt.title("Difference between wrist timestamp and base timestamp\n{} trials".format(len(diff)))
        plt.xlabel("Drift in ms")
        plt.ylabel("Frequency")
        plt.show()


if __name__ == '__main__':
    # filename = "2021-04-15_23;29;55"
    # same_time_experiment_reconstruct(filename)
    get_all_timestamps(hist_limit=None, plot_greater_than=None)