import matplotlib.pyplot as plt
import os
import pandas as pd
import serial
import numpy as np
import io

# use ggplot style for more sophisticated visuals
plt.style.use('ggplot')


def live_plotter(x_vec, y1_data, line1, identifier='', pause_time=0.01):
    if line1 == []:
        # this is the call to matplotlib that allows dynamic plotting
        plt.ion()
        fig = plt.figure(figsize=(13, 6))
        ax = fig.add_subplot(111)
        # create a variable for the line so we can later update it
        line1, = ax.plot(x_vec, y1_data, '-o', alpha=0.8)
        # update plot label/title
        plt.ylabel('Acceleration Label')
        plt.title('Acceleration Output: {}'.format(identifier))
        plt.ylim([1000, 1200])
        plt.show()

    # after the figure, axis, and line are created, we only need to update the y-data
    line1.set_ydata(y1_data)
    # adjust limits if new data goes beyond bounds
    if np.min(y1_data) <= line1.axes.get_ylim()[0] or np.max(y1_data) >= line1.axes.get_ylim()[1]:
        plt.ylim([np.min(y1_data) - np.std(y1_data), np.max(y1_data) + np.std(y1_data)])
    # this pauses the data so the figure/axis can catch up - the amount of pause can be altered above
    plt.pause(pause_time)

    # return line so we can update it again in the next iteration
    return line1

def plot_dynamically(serial_port):
    size = 100
    x_vec = np.linspace(0,1,size+1)[0:-1]
    y_vec = np.random.randn(len(x_vec))
    line1 = []
    while True:
        sample = read_one_sample_from_board(serial_port)
        accel = get_accel_value(sample)
        y_vec[-1] = accel
        line1 = live_plotter(x_vec,y_vec,line1)
        y_vec = np.append(y_vec[1:],0.0)


def plot_acceleration_from_csv(csv_name= ""):
    path = os.path.join(os.getcwd(), "demo", "data.csv")
    df = pd.read_csv(path)
    #print(df.columns)
    timestamps = list(df['Timestamp'].values)
    acceleration = list(df['Acceleration'].values)

    #print(timestamps[len(timestamps)-1])
    plt.figure(0)
    #plt.xlim(0, timestamps[-1])
    plt.plot(timestamps, acceleration, label='acceleration')
    plt.title('Acceleration')
    plt.xlabel('Time in ms')
    plt.ylabel('Relative Acceleration')
    plt.legend()
    plt.show()


def read_one_sample_from_board(serial_port):
    for i in range (5):
        data = serial_port.readline()
        data = str(data[1:-2])[2:-1]
        return data


def get_accel_timestamp_value(sample):
    #returns (accel, timestamp)
    try:
        sample = sample.split(",")
        return float(sample[-2]), float(sample[-1])
    except:
        pass
        return None


def get_all_accel_values(sample):
    #returns (accel, timestamp)
    sample = sample.split(",")
    return float(sample[-5]), float(sample[-4]), float(sample[-3]), float(sample[-1]), float(sample[-2])


def setup_serial():
    com5 = serial.Serial()
    com5.port = 'COM6'
    com5.baudrate = 115200
    com5.open()
    return com5


def plot_acceleration_from_board(serial_port = None, num_samples = 2950):
    if serial_port is None:
        serial_port = setup_serial()

    accel_values = []
    timestamp_values = []
    for i in range(num_samples):
        data = None
        legit_read = False
        while(data is None and not legit_read):
            sample = read_one_sample_from_board(serial_port)
            data = get_accel_timestamp_value(sample)
            if data is not None:
                accel, timestamp = data
                if accel < 10:
                    legit_read = True
        accel_values.append(accel/1.18)
        timestamp_values.append(timestamp)
    plt.figure(0)
    # plt.xlim(0, timestamps[-1])
    plt.ylim(0.25, 1.75)
    avg = sum(timestamp_values)/len(timestamp_values)
    x_axis = [i*avg for i in range(len(accel_values))]
    plt.plot(x_axis, accel_values, label='acceleration')
    plt.title('Acceleration vs Time, {} samples'.format(num_samples))
    plt.xlabel('Time in ms')
    plt.ylabel('Acceleration in Gs')
    plt.legend()
    plt.show()
    print(timestamp_values)
    print(accel_values)
    print(len(accel_values))

def plot_acceleration_xyx_from_board(serial_port = None, num_samples = 1000):
    if serial_port is None:
        serial_port = setup_serial()

    accel_values = []
    timestamp_values = []
    x_values = []
    y_values = []
    z_values = []
    for i in range(num_samples):
        try:
            sample = read_one_sample_from_board(serial_port)
            x, y, z, accel, timestamp = get_all_accel_values(sample)
            accel_values.append(accel)
            timestamp_values.append(timestamp)
            x_values.append(x)
            y_values.append(y)
            z_values.append(z)
            if (i != 0):
                timestamp_values[i] -= timestamp_values[0]
        except:
            pass

    fig, axs = plt.subplots(3, sharex=True, sharey=True)
    fig.suptitle("X, Y, and Z acceleration")
    axs[0].plot(timestamp_values, x_values)
    axs[1].plot(timestamp_values, y_values)
    axs[2].plot(timestamp_values, z_values)
    plt.show()

plot_acceleration_from_board()
#plot_acceleration_xyx_from_board()