from __future__ import annotations
from abc import ABC
import _pygrenade_vx_network as grenade


class Mechanism(ABC):
    '''
    Abstract base class for all Mechanisms.
    '''
    def __init__(self):
        '''
        Initialize mechanism.
        '''
        self.parameters = {}

    def to_grenade(self):
        '''
        Create the mechansim in grenade.
        '''
        raise NotImplementedError()

    def copy(self):
        '''
        Create a copy of the mechanism.
        '''
        raise NotImplementedError()


class MembraneCapacitance(Mechanism):
    '''
    Mechanism that defines the membrane capacitance of a compartment.
    '''

    def __init__(self, capacitance: int = 0):
        '''
        Initialize membrane capacitance with given capacitance.

        :param capacitance: Capacitance of the membrane.
        '''
        super().__init__()
        self.parameters["capacitance"] = capacitance

    def to_grenade(self) -> grenade.MechanismCapacitance:
        '''
        Create the mechanism in grenade.

        :return: Grenade object of the mechanism.
        '''
        parameter_interval_capacitance = grenade.ParameterIntervalDouble(
            self.parameters["capacitance"], self.parameters["capacitance"])

        parameterization = grenade.MechanismCapacitance.\
            ParameterSpace.Parameterization(self.parameters["capacitance"])
        parameter_space = grenade.MechanismCapacitance.ParameterSpace(
            parameter_interval_capacitance, parameterization)

        return grenade.MechanismCapacitance(parameter_space)

    def copy(self) -> MembraneCapacitance:
        '''
        Create copy of the mechanism.

        :return: Copy of the mechanism.
        '''
        return MembraneCapacitance(
            capacitance=self.parameters["capacitance"])


class CurrentBasedSynapse(Mechanism):
    '''
    Mechanism that defines the parameters
    of a current based synaptic input.
    '''

    def __init__(self, current: int = 0, time_constant: int = 0):
        '''
        Initialize a current based synapse.

        :param current: Strength of the synaptic input.
        :param time_constant: Time constant of the synaptic input.
        '''
        super().__init__()
        self.parameters["current"] = current
        self.parameters["time_constant"] = time_constant

    def to_grenade(self) -> grenade.MechanismSynapticInputCurrent:
        '''
        Create the mechanism in grenade.

        :return: Grenade object of the mechanism.
        '''
        parameter_interval_current = grenade.ParameterIntervalDouble(
            self.parameters["current"], self.parameters["current"])
        parameter_interval_time_const = grenade.ParameterIntervalDouble(
            self.parameters["time_constant"], self.parameters["time_constant"])

        parameterization = grenade.MechanismSynapticInputCurrent.\
            ParameterSpace.Parameterization(
                self.parameters["current"], self.parameters["time_constant"])
        parameter_space = grenade.MechanismSynapticInputCurrent.\
            ParameterSpace(parameter_interval_current,
                           parameter_interval_time_const,
                           parameterization)

        return grenade.MechanismSynapticInputCurrent(parameter_space)

    def copy(self) -> CurrentBasedSynapse:
        '''
        Create copy of the mechanism.

        :return: Copy of the mechanism.
        '''
        return CurrentBasedSynapse(
            current=self.parameters["current"],
            time_constant=self.parameters["time_constant"])


class ConductanceBasedSynapse(Mechanism):
    '''
    Mechanism that defines the parameters of a
    conductance based synaptic input.
    '''

    def __init__(self,
                 conductance: int = 0,
                 potential: int = 0,
                 time_constant: int = 0):
        '''
        Initialize a conductance based synapse.

        :param conductance: Strength of the synaptic input.
        :param potential: Reversal potential.
        :param time_constant: Time constant of the synaptic input.
        '''
        super().__init__()
        self.parameters["conductance"] = conductance
        self.parameters["potential"] = potential
        self.parameters["time_constant"] = time_constant

    def to_grenade(self) -> grenade.MechanismSynapticInputConductance:
        '''
        Create the mechanism in grenade.

        :return: Grenade object of the mechanism.
        '''
        parameter_interval_conductance = grenade.ParameterIntervalDouble(
            self.parameters["conductance"], self.parameters["conductance"])
        parameter_interval_potential = grenade.ParameterIntervalDouble(
            self.parameters["potential"], self.parameters["potential"])
        parameter_interval_time_const = grenade.ParameterIntervalDouble(
            self.parameters["time_constant"], self.parameters["time_constant"])

        parameterization = grenade.MechanismSynapticInputConductance.\
            ParameterSpace.Parameterization(
                self.parameters["conductance"], self.parameters["potential"],
                self.parameters["time_constant"])
        parameter_space = grenade.MechanismSynapticInputConductance.\
            ParameterSpace(
                parameter_interval_conductance, parameter_interval_potential,
                parameter_interval_time_const, parameterization)

        mechanism_conductance_grenade = \
            grenade.MechanismSynapticInputConductance(parameter_space)
        return mechanism_conductance_grenade

    def copy(self) -> ConductanceBasedSynapse:
        '''
        Create copy of the mechanism.

        :return: Copy of the mechanism.
        '''
        return ConductanceBasedSynapse(
            conductance=self.parameters["conductance"],
            potential=self.parameters["potential"],
            time_constant=self.parameters["time_constant"])
