import matplotlib.pyplot as plt
import os
import pandas as pd

def plot_acceleration(csv_name):
    PATH = os.getcwd()
    csv_path = os.path.join(PATH, csv_name)
    df = pd.read_csv(csv_path)
    print(df.columns)
    timestamps = list(df["time"].values)
    acceleration = list(df[" acceleration"].values)


    plt.figure(0)
    plt.plot(timestamps, acceleration, label='acceleration')
    plt.title('Acceleration')
    plt.xlabel('Time in ms')
    plt.ylabel('Relative Acceleration')
    plt.legend()
    plt.show()

plot_acceleration("15s_table_demo_shake_board.csv")
