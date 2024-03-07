import pylogging as logger
from pygrenade_vx import common
from pygrenade_vx import execution
from pygrenade_vx import signal_flow
from pygrenade_vx import network

if logger.get_root().get_number_of_appenders() == 0:
    logger.reset()
    logger.default_config(level=logger.LogLevel.WARN)
