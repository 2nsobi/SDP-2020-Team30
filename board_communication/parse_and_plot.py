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


# timestamp = 100
array1 = [(1, 1), (3, 2), (6, 3), (10, 7)]
array2 = [(3, 2), (4, 3), (5, 4), (9, 14)]

my_data = [array1, array2]
t_stamp = 100

parser(my_data, t_stamp)

