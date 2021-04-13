import logging


class Logger:
    __logger_instance = None

    @staticmethod
    def get_instance():
        if Logger.__logger_instance is None:
            logging.basicConfig(format=('%(filename)s: '
                                        '%(levelname)s: '
                                        '%(funcName)s(): '
                                        '%(lineno)d:\t'
                                        '%(message)s'))
            Logger.__logger_instance = logging.getLogger(__name__)
        return Logger.__logger_instance
