import os
import matplotlib.pyplot as plt
import numpy


def parse_same_time_experiment(experiment_folder):
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

    fig, ax = plt.subplots(2)
    ax[0].plot(wrist_x, wrist_y, label="wrist")
    ax[0].plot(base_x, base_y, label="base")

    ax[0].set_xlabel("Beacon Timestamp in ms")
    ax[0].set_ylabel("Relative ADC readings")
    ax[0].legend()

    wrist_derivative = numpy.gradient(wrist_y)
    base_derivative = numpy.gradient(base_y)

    ax[1].plot(wrist_x, wrist_derivative, label="wrist_derivative")
    ax[1].plot(base_x, base_derivative, label="base_derivative")

    ax[1].set_xlabel("Beacon Timestamp in ms")
    ax[1].set_ylabel("Derivative of ADC Readings")
    ax[1].legend()
    plt.show()

def same_time_experiment_reconstruct(folder = "2021-04-15_23;29;40"):
    cwd = os.getcwd()
    cwd = os.path.split(cwd)[0]
    experiment_folder = os.path.join(cwd, "same_time_trigger_data", folder)
    parse_same_time_experiment(experiment_folder)

if __name__ == '__main__':
    filename = "2021-04-15_23;29;55"
    same_time_experiment_reconstruct(filename)