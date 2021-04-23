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

    if wrist_ts is None or base_ts is None:
        return calculate_timestamps(wrist_x, wrist_der, base_x, base_der, threshold=threshold-0.1)

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
    if folder == "same_time":
        folder = os.path.join(os.getcwd(), "same_time_trigger_data")
    if folder == "slomo":
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

def analyze_slomo(hist_limit = None, diff_threshold = 100, plot = True):
    # hist_limit: (int) only include differences in the histogram of less than hist_limit
    # plot: (bool) if true, plot graphs of experiments where the system was wrong according to the video
    safe = None
    folder = os.path.join(os.getcwd(), "slo_mo_testing")
    experiments = os.listdir(folder)
    print("{} slomo experiments".format(len(experiments)))

    total_correct = 0
    diff_between_est_actual = []
    under_35_total = 0
    under_35_correct = 0
    over_35_total = 0
    over_35_correct = 0

    for experiment in experiments:
        exp_folder = os.path.join(folder, experiment)
        wrst_x, wrst_y, wrst_der, base_x, base_y, base_der = parse_same_time_experiment(exp_folder,
                                                                                        plot=False)
        wrist_ts, base_ts = calculate_timestamps(wrst_x, wrst_der, base_x, base_der)
        exp_diff = wrist_ts - base_ts
        if exp_diff > 0:    #base ts is lower, base came first, runner is safe
            safe = True
            pred_module_first = "base"
        else:
            safe = False
            pred_module_first = "wrist"
        for exp_file in os.listdir(exp_folder):
            if exp_file.find("video")>=0:
                video_file = os.path.join(exp_folder,exp_file)
        module_first, elapsed = retrieve_slomo_data(video_file)
        if abs(elapsed) > 35:
            over_35_total +=1
        else:
            under_35_total +=1
        diff_in_est_time = abs(exp_diff) - abs(elapsed)
        trial_number = ""
        for i in os.path.split(video_file)[1]:
            if i.isdigit():
                trial_number += i
            else:
                break
        if hist_limit is not None:
            if abs(diff_in_est_time) < hist_limit:
                diff_between_est_actual.append(diff_in_est_time)
        else:
            diff_between_est_actual.append(diff_in_est_time)
        if module_first == pred_module_first:
            total_correct +=1
            if abs(elapsed) > 35:
                over_35_correct +=1
            else:
                under_35_correct +=1
        else:
            print("Incorrect prediction for experiment {}".format(experiment))
            print("Trial number {}".format(trial_number))
            if plot:
                plot_with_ts(wrst_x, wrst_y, wrst_der, wrist_ts,
                             base_x, base_y, base_der, base_ts, exp_folder)
                print("\tDifference in est time for trial {} is {}".format(trial_number, diff_in_est_time))
                print("\tSystem est time: {:4f}".format(exp_diff))
                print("\tVideo est time: {:4f}".format(elapsed))
        if diff_threshold is not None:
            if abs(diff_in_est_time) > diff_threshold:
                print("Difference in est time for trial {} is {}".format(trial_number, diff_in_est_time))
                print("System est time: {:4f}".format(exp_diff))
                print("Video est time: {:4f}".format(elapsed))
                plot_with_ts(wrst_x, wrst_y, wrst_der, wrist_ts,
                             base_x, base_y, base_der, base_ts, exp_folder)

    print("{} total correct out of {} trials, {:4f}%".format(total_correct, len(experiments),
                                                             100*total_correct/len(experiments)))
    print("UNDER 35 MS: {} total correct out of {} trials, {:4f}%".format(under_35_correct, under_35_total,
                                                             100 * under_35_correct /under_35_total))
    print("OVER 35 MS: {} total correct out of {} trials, {:4f}%".format(over_35_correct, over_35_total,
                                                                          100 * over_35_correct / over_35_total))


    plt.hist(diff_between_est_actual, bins=30)
    plt.title("Difference between TrueBase Time Difference "
              "and Slo-mo Time Difference\n{} trials".format(len(diff_between_est_actual)))
    plt.xlabel("Difference in ms")
    plt.ylabel("Frequency")
    plt.show()

def create_video_txt_files_slomo():
    folder = os.path.join(os.getcwd(), "slo_mo_testing")
    experiments = os.listdir(folder)
    print("{} slomo experiments".format(len(experiments)))
    for i in range(len(experiments)):
        exp_folder = os.path.join(folder, experiments[i])
        exp_files = os.listdir(exp_folder)
        for exp_file in exp_files:
            if exp_file.find("video")>=0:
                remove_file = os.path.join(exp_folder, exp_file)
                os.remove(remove_file)
        fname = "{}video.txt".format(str(i+1))
        fpath = os.path.join(exp_folder,fname)
        f = open(fpath, 'w+')
        f.close()


def retrieve_slomo_data(file):
    f = open(file,'r')
    a = f.readline()
    b = a.split(",")
    module_first = b[0]
    frame_rate = int(b[1])
    #frame_rate = 25
    frames_between = int(b[2])
    elapsed = 1000*float(frames_between)/frame_rate
    return module_first, elapsed

def test_slomo_video_retrieval():
    file = os.path.join(os.getcwd(), "slo_mo_testing", "2021-04-20_12;09;49", "1video.txt")
    b = retrieve_slomo_data(file)
    print(b)

if __name__ == '__main__':
    # filename = "2021-04-15_23;29;55"
    # same_time_experiment_reconstruct(filename)
    #get_all_timestamps(hist_limit=None, plot_greater_than=50)
    #create_video_txt_files_slomo()
    analyze_slomo(hist_limit=300, plot=True, diff_threshold=None)
    #test_slomo_video_retrieval()
