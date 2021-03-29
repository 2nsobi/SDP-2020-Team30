import matplotlib.pyplot as plt
import serial
import os
from board_communication.test_local_clocks import str_date

PORT = '/dev/ttyACM0'

# use ggplot style for more sophisticated visuals
plt.style.use('ggplot')


def read_one_beac_hw_local_sample(serial_port):
    # reads one line from the serial port and returns (beacon_ts, hw_timestamp, local_ts)
    # lines on the serial port should be formatted as beacon_ts,hw_timestamp,local_ts
    try:
        original = str(serial_port.readline())
        data = original.replace("'", "")
        data = data.replace("b", "")
        data = data.strip("\\n")
        data = data.split(",")
        return int(data[0]), int(data[1]), int(data[2])
    except Exception as e:
        print(e)
        print(original)



def setup_serial():
    com = serial.Serial()
    com.port = PORT
    com.baudrate = 115200
    com.open()
    return com

def tera_term():
    c = setup_serial()
    while(1):
        x = str(c.readline())
        print(x)


def test_hw_timestamps():
    c = setup_serial()

    data = []   # array of 3-tuples (beac_ts, hw_timestamp, local_ts)
    i = 1
    while(len(data) < 30):
        x = read_one_beac_hw_local_sample(c)
        if x is not None:
            data.append(x)
            print(i, x)
            i += 1
    save_hw_ts_test(data)


def save_hw_ts_test(data):
    # data is an array of 3-tuples (beacon_ts, hw_ts, local_ts)
    # this function stores <data> into a txt file in csv format
    root_folder = os.path.join(os.getcwd(), "hw_timestamp_data")
    if not os.path.exists(root_folder):
        os.mkdir(root_folder)
    date = str_date()
    filename = str(date).replace(" ", "_").split(".")[0].replace(":", ";") + ".txt"
    filepath = os.path.join(root_folder, filename)
    f = open(filepath, "w+")
    f.write("beacon_ts,hw_timestamp,local_ts")
    for x in data:
        line = "\n{},{},{}".format(x[0],x[1],x[2])
        f.write(line)
    f.close()



if __name__ == '__main__':
    #tera_term()
    test_hw_timestamps()