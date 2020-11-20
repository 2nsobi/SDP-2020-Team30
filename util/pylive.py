import matplotlib.pyplot as plt
import numpy as np

# use ggplot style for more sophisticated visuals
plt.style.use('ggplot')


def live_plotter(x_vec, y1_data, line1, identifier='', pause_time=0.1):
    if line1 == []:
        # this is the call to matplotlib that allows dynamic plotting
        plt.ion()
        fig = plt.figure(figsize=(13, 6))
        ax = fig.add_subplot(111)
        # create a variable for the line so we can later update it
        line1, = ax.plot(x_vec, y1_data, '-o', alpha=0.8)
        # update plot label/title
        plt.ylabel('Y Label')
        plt.title('Title: {}'.format(identifier))
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


# the function below is for updating both x and y values (great for updating dates on the x-axis)
def live_plotter_xy(x_vecs, y_vecs, lines, line_count, x_label='test x', y_labels=None, title='test title',
                    pause_time=0.01, dynamic_y=True, y_ranges=None):
    if lines == []:
        plt.ion()
        fig, axs = plt.subplots(line_count, sharex=True, sharey=False, figsize=(13, 6))

        fig.text(0.5, 0.04, x_label, ha='center', fontsize='x-large')
        # fig.text(0.04, 0.5, y_label, va='center', rotation='vertical', fontsize='x-large')

        for i in range(line_count):
            if line_count > 1:
                line, = axs[i].plot(x_vecs[i], y_vecs[i], 'r-o', alpha=0.8)
            else:
                line, = axs.plot(x_vecs[i], y_vecs[i], 'r-o', alpha=0.8)

            if y_labels is not None:
                line.axes.set_ylabel(y_labels[i])
            else:
                line.axes.set_ylabel('test y')

            if not dynamic_y:
                line.axes.set_ylim(y_ranges[i][0], y_ranges[i][1])

            lines.append(line)

        fig.suptitle(title, fontsize='xx-large', fontweight='bold')
        plt.show()


    for i in range(line_count):
        lines[i].set_data(x_vecs[i], y_vecs[i])
        lines[i].axes.set_xlim(np.min(x_vecs[i]), np.max(x_vecs[i]))

        if dynamic_y:
            # if np.min(y_vecs[i]) <= lines[i].axes.get_ylim()[0] or np.max(y_vecs[i]) >= lines[i].axes.get_ylim()[1]:
            lines[i].axes.set_ylim([np.min(y_vecs[i]) - np.std(y_vecs[i]), np.max(y_vecs[i]) + np.std(y_vecs[i])])

    plt.pause(pause_time)

    return lines
