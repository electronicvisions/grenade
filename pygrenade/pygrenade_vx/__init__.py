import pylogging as logger
import pygrenade_vx.execution
import pygrenade_vx.signal_flow
import pygrenade_vx.network
import pygrenade_vx.logical_network

if logger.get_root().get_number_of_appenders() == 0:
    logger.reset()
    logger.default_config(level=logger.LogLevel.WARN)
