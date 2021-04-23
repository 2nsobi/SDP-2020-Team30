"""
set these env vars too (use 'set' on windows and 'export' on unix):
- FLASK_APP=flask_app.py (to tell the terminal the application to work)
- FLASK_ENV=development (for debug mode)
"""
import datetime
import os

import numpy
from flask import Flask, render_template, render_template_string, Blueprint
import matplotlib.pyplot as plt
import mpld3
from mpld3 import plugins
import numpy as np
import board_communication.system_commands as sys_cmds
from board_communication.reconstruct_experiments import calculate_timestamps

views = Blueprint('auth', __name__)
loading_plots = False
modules_plot_html = ""


@views.route('/')
def index():
    return render_template('index.html')


@views.route('/dummy-page')
def dummy_page():
    return ""


@views.route('/get-modules-plot')
def get_modules_plot():
    global modules_plot_html
    return render_template_string(modules_plot_html)


@views.route('/load-modules-plot', methods=['POST'])
def load_modules_plot(save=False, relevant_data=False, vertical_lines=True):
    global loading_plots, modules_plot_html

    if loading_plots:
        return ""
    loading_plots = True

    res = sys_cmds.get_data_from_boards()

    if res is not None:
        wrist_x_axis, wrist_y, base_x_axis, base_y = res

        wrist_1d = [x[0] for x in wrist_y.tolist()]
        base_1d = [x[0] for x in base_y.tolist()]
        print(len(wrist_1d))
        print(len(base_1d))
        wrist_derivative = numpy.gradient(wrist_1d)
        base_derivative = numpy.gradient(base_1d)

        wrist_ts, base_ts = calculate_timestamps(wrist_x_axis, wrist_derivative,
                                                 base_x_axis, base_derivative, threshold=0.6)

        if relevant_data:
            wrist_index = wrist_x_axis.index(wrist_ts)
            wrist_lower = wrist_index - 10 if wrist_index - 10 >= 0 else 0
            wrist_higher = wrist_index + 30 if wrist_index + 30 < len(wrist_x_axis) else len(wrist_x_axis) - 1

            base_index = base_x_axis.index(base_ts)
            base_lower = base_index - 10 if base_index - 10 >= 0 else 0
            base_higher = base_index + 30 if base_index + 30 < len(base_x_axis) else len(base_x_axis)

            lower = min(wrist_lower, base_lower)
            higher = max(wrist_higher, base_higher)
            if higher > len(wrist_x_axis) or higher > len(base_x_axis):
                higher = min(len(wrist_x_axis, len(base_x_axis)))

            wrist_x_axis = wrist_x_axis[lower:higher]
            wrist_y = wrist_y[lower:higher]
            wrist_derivative = wrist_derivative[lower:higher]

            base_x_axis = base_x_axis[lower:higher]
            base_y = base_y[lower:higher]
            base_derivative = base_derivative[lower:higher]

        fig, ax = plt.subplots(2)

        # plugins.clear(fig)  # clear all plugins from the figure
        # plugins.connect(fig, plugins.Reset(), plugins.BoxZoom(), plugins.Zoom())

        ax[0].plot(wrist_x_axis, wrist_y, label="wrist")
        ax[0].plot(base_x_axis, base_y, label="base")
        try:
            if vertical_lines:
                ax[0].vlines(wrist_ts, ymin=0, ymax=1, colors='g',
                             linestyles='dashed', label='wrist_ts')
                ax[0].vlines(base_ts, ymin=0, ymax=1, colors='r',
                             linestyles='dashed', label='base_ts')
        except Exception as e:
            print(wrist_ts)
            print(base_ts)
            raise e

        # ax[0].set_xlabel("Beacon Timestamp in ms")
        ax[0].set_ylabel("Relative ADC readings")
        ax[0].legend(loc='upper right')

        ax[1].plot(wrist_x_axis, wrist_derivative, label="wrist_derivative")
        ax[1].plot(base_x_axis, base_derivative, label="base_derivative")
        if vertical_lines:
            ax[1].vlines(wrist_ts, ymin=-0.5, ymax=0.5, colors='g',
                         linestyles='dashed', label='wrist_ts')
            ax[1].vlines(base_ts, ymin=-0.5, ymax=0.5, colors='r',
                         linestyles='dashed', label='base_ts')

        ax[1].set_xlabel("Beacon Timestamp in ms")
        ax[1].set_ylabel("Derivative of ADC Readings")
        ax[1].legend(loc='upper right')
        # plt.show()
        out = None
        if wrist_ts < base_ts:
            out = "RUNNER OUT"
        elif wrist_ts > base_ts:
            out = "RUNNER SAFE"
        else:
            out = "TIE GOES TO THE RUNNER"
        ax[0].set_title("Wrist_ts: {:.2f} ms, Base_ts: {:.2f} ms, "
                        "time diff: {:.2f} ms, suggested outcome: {}"
                        .format(wrist_ts, base_ts, wrist_ts - base_ts,out))

        if save:
            # save fig to image to disk---------------------------------------------------

            # if root folder does not exist, save it
            same_time_data_folder = os.path.join(os.getcwd(), "slo_mo_testing")
            if not os.path.exists(same_time_data_folder):
                os.mkdir(same_time_data_folder)

            # save folder is in the format year month day hour min sec
            a = str(datetime.datetime.now()).split(".")[0]
            b = a.replace(" ", "_")
            folder_name = b.replace(":", ";")
            folder_path = os.path.join(same_time_data_folder, folder_name)

            # make the folder
            os.mkdir(folder_path)
            fig.savefig(os.path.join(folder_path, "output.png"))

            # save base module data
            base_txt_file = os.path.join(folder_path, "base_data.txt")
            f = open(base_txt_file, 'w')
            base_x = str(base_x_axis[0])
            for i in range(1, len(base_x_axis)):
                base_x += "," + str(base_x_axis[i])
            f.write(str(base_x))
            f.write("\n")
            base_y_arr = [x[0] for x in base_y.tolist()]
            f.write(str(base_y_arr))
            f.close()

            # save wrist module data
            wrist_txt_file = os.path.join(folder_path, "wrist_data.txt")
            f = open(wrist_txt_file, 'w')
            wrist_x = str(wrist_x_axis[0])
            for i in range(1, len(wrist_x_axis)):
                wrist_x += "," + str(wrist_x_axis[i])
            f.write(str(wrist_x))
            f.write("\n")
            wrist_y_arr = [x[0] for x in wrist_y.tolist()]
            f.write(str(wrist_y_arr))
            f.close()
            # ----------------------------------------------------------------

        modules_plot_html = mpld3.fig_to_html(fig)

    loading_plots = False
    return ""


@views.route('/loading-modules-plot')
def loading_modules_plot():
    global loading_plots
    if loading_plots:
        return "true"
    return "false"


@views.route('/loader-animation')
def loader_animation():
    return render_template('loader.html')


@views.route('/failed-loading-plot')
def failed_loading_plot():
    return render_template_string("<div style=\"font-family: \"sans-serif\"\">Failed to Load Module Data Plot</div>")
