"""
set these env vars too (use 'set' on windows and 'export' on unix):
- FLASK_APP=app.py (to tell the terminal the application to work)
- FLASK_ENV=development (for debug mode)
"""
from flask import Flask, render_template, render_template_string, Blueprint
import matplotlib.pyplot as plt
import mpld3
import numpy as np
import board_communication.system_commands as sys_cmds

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
def load_modules_plot():
    global loading_plots, modules_plot_html

    if loading_plots:
        return ""
    loading_plots = True

    res = sys_cmds.get_data_from_boards()

    if res is not None:
        wrist_x_axis, wrist_y, base_x_axis, base_y = res

        # print(len(wrist_x_axis), len(wrist_y))
        # print(len(base_x_axis), len(base_y))

        fig, ax = plt.subplots()
        ax.plot(wrist_x_axis, wrist_y, label="wrist")
        ax.plot(base_x_axis, base_y, label="base")

        ax.set_xlabel("Beacon Timestamp in ms")
        ax.set_ylabel("Relative ADC readings")
        ax.legend()

        modules_plot_html = mpld3.fig_to_html(fig)

    loading_plots = False
    return ""


@views.route('/loading-modules-plot')
def loading_modules_plot():
    global loading_plots
    if loading_plots:
        return "true"
    return "false"
