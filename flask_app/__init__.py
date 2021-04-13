import os

from flask import Flask
import board_communication.system_commands as sys_cmds
from . import main_app


def create_app(test_config=None):
    # create and configure the app
    app = Flask(__name__, instance_relative_config=True)

    # ensure the instance folder exists
    try:
        os.makedirs(app.instance_path)
    except OSError:
        pass

    sys_cmds.initialize_base_station()

    app.register_blueprint(main_app.views)

    return app
