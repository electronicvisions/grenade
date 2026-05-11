from __future__ import annotations
from copy import copy, deepcopy
from typing import Dict, Union
from abc import ABC, abstractmethod

import _pygrenade_vx_network_abstract as grenade
from _pygrenade_vx_network_abstract import AnalogValueInterval, \
    CapacitanceInterval, TimeInterval, ParameterIntervalDouble
import _pygrenade_vx_network as grenade_network


class Mechanism(ABC):
    '''
    Abstract base class for all Mechanisms.
    '''
    def __init__(self):
        '''
        Initialize mechanism.
        '''
        self.parameters = {}

    @abstractmethod
    def to_grenade(self):
        '''
        Create the mechanism in grenade.
        '''
        raise NotImplementedError()

    @classmethod
    @abstractmethod
    def construct_parameter_space(
        cls,
        lower_limits: Dict[str, float],
        upper_limits: Dict[str, float]
    ) -> grenade.Mechanism.ParameterSpace:
        """
        Construct a parameter space with the given limits.

        :param lower_limits: Lower limits of the parameter space.
            The keys are the names of the parameters.
        :param upper_limits: Upper limits of the parameter space.
            The keys are the names of the parameters.
        :return: Parameter space with the given limits.
        """
        raise NotImplementedError()

    @classmethod
    @abstractmethod
    def construct_parameterization(
        cls,
        values: Dict[str, Union[float, int]]
    ) -> grenade.Mechanism.Parameterization:
        """
        Construct a parameterization for the given mechanism.

        :param values: Values for the different parameters.
            The keys are the names of the parameters.
        :return: Parameterization.
        """
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
        return grenade.MechanismCapacitance(enable_analog_readout=True)

    @classmethod
    def construct_parameter_space(
        cls,
        lower_limits: Dict[str, float],
        upper_limits: Dict[str, float]
    ) -> grenade.MechanismCapacitance.ParameterSpace:
        intervals = [CapacitanceInterval(float(low), float(high))
                     for low, high in zip(lower_limits['capacitance'],
                                          upper_limits['capacitance'])]
        return grenade.MechanismCapacitance.ParameterSpace(intervals)

    @classmethod
    def construct_parameterization(
        cls,
        values: Dict[str, Union[float, int]]
    ) -> grenade.MechanismCapacitance.ParameterSpace.Parameterization:
        return grenade.MechanismCapacitance.ParameterSpace.\
            Parameterization([float(val) for val in values['capacitance']])

    def copy(self) -> MembraneCapacitance:
        '''
        Create copy of the mechanism.

        :return: Copy of the mechanism.
        '''
        return MembraneCapacitance(
            capacitance=self.parameters["capacitance"])


class CurrentBasedSynapse(Mechanism):
    '''
    Current-based synapse.

    Current-based synapse with a synaptic time constant and an input
    strength. If all synaptic inputs on a chip have the same strength, the
    synaptic current injected on the membrane is equalized between
    different synaptic inputs. Otherewise, the strength is directly
    translated to `i_bias_gm` of the synaptic input.
    '''
    def __init__(self,
                 receptor_type: str = "excitatory",
                 *,
                 strength: int = 500,
                 time_constant: float = 10e-6,
                 global_strength: int = 600):
        '''
        Initialize a current based synapse.

        :param strength: Strength of the synaptic input.
        :param time_constant: Time constant (in s) of the synaptic input.
        :param global_strength: Gloabal scaling of the synaptic input
            strength. Translates to `synapse_dac_bias`. Has to be the same
            for all synaptic inputs on a chip.
        '''
        super().__init__()
        if receptor_type == "excitatory":
            self.receptor_type = grenade_network.Receptor.Type.excitatory
        elif receptor_type == "inhibitory":
            self.receptor_type = grenade_network.Receptor.Type.inhibitory
        else:
            raise RuntimeError("Only excitatory and inhibitory synapses are "
                               "supported.")

        self.parameters["strength"] = strength
        self.parameters["time_constant"] = time_constant
        self.parameters["global_strength"] = global_strength

    def to_grenade(self) -> grenade.MechanismSynapticInputCurrent:
        '''
        Create the mechanism in grenade.

        :return: Grenade object of the mechanism.
        '''
        return grenade.MechanismSynapticInputCurrent(
            receptor_type=self.receptor_type, enable_analog_readout=True)

    @classmethod
    def construct_parameter_space(
        cls,
        lower_limits: Dict[str, float],
        upper_limits: Dict[str, float]
    ) -> grenade.MechanismSynapticInputCurrent.ParameterSpace:

        strengths = [AnalogValueInterval(int(low), int(high))
                     for low, high in zip(lower_limits["strength"],
                                          upper_limits["strength"])]
        global_strengths = [
            AnalogValueInterval(int(low), int(high)) for low, high in
            zip(lower_limits["global_strength"],
                upper_limits["global_strength"])]

        time_constants = [TimeInterval(float(low), float(high))
                          for low, high in zip(lower_limits["time_constant"],
                                               upper_limits["time_constant"])]
        return grenade.MechanismSynapticInputCurrent.ParameterSpace(
            i_synin_gm=strengths, synapse_dac_bias=global_strengths,
            time_constant=time_constants)

    @classmethod
    def construct_parameterization(
        cls,
        values: Dict[str, Union[float, int]]
    ) -> grenade.MechanismSynapticInputCurrent.Parameterization:
        return grenade.MechanismSynapticInputCurrent.ParameterSpace.\
            Parameterization(
                i_synin_gm=[int(s) for s in values["strength"]],
                synapse_dac_bias=[int(s) for s in values["global_strength"]],
                time_constant=[float(t) for t in values["time_constant"]])

    def copy(self) -> CurrentBasedSynapse:
        '''
        Create copy of the mechanism.

        :return: Copy of the mechanism.
        '''
        syn = CurrentBasedSynapse()
        syn.parameters = deepcopy(self.parameters)
        syn.receptor_type = copy(self.receptor_type)
        return syn


class ConductanceBasedSynapse(Mechanism):
    '''
    Mechanism that defines the parameters of a
    conductance based synaptic input.
    '''

    def __init__(self,
                 receptor_type: str = "excitatory",
                 *,
                 strength: int = 500,
                 potential: int = 800,
                 reference: int = 400,
                 time_constant: int = 10e-6,
                 global_strength: int = 600):
        '''
        Initialize a conductance based synapse.

        :param strength: Strength of the synaptic input.
        :param potential: Reversal potential.
        :param time_constant: Time constant of the synaptic input.
        '''
        super().__init__()

        if receptor_type == "excitatory":
            self.receptor_type = grenade_network.Receptor.Type.excitatory
        elif receptor_type == "inhibitory":
            self.receptor_type = grenade_network.Receptor.Type.inhibitory
        else:
            raise RuntimeError("Only excitatory and inhibitory synapses are "
                               "supported.")

        self.parameters["strength"] = strength
        self.parameters["potential"] = potential
        self.parameters["reference"] = reference
        self.parameters["time_constant"] = time_constant
        self.parameters["global_strength"] = global_strength

    def to_grenade(self) -> grenade.MechanismSynapticInputConductance:
        '''
        Create the mechanism in grenade.

        :return: Grenade object of the mechanism and its parameter space.
        '''
        return grenade.MechanismSynapticInputConductance(
            receptor_type=self.receptor_type, enable_analog_readout=True)

    @classmethod
    def construct_parameter_space(
        cls,
        lower_limits: Dict[str, float],
        upper_limits: Dict[str, float]
    ) -> grenade.MechanismSynapticInputConductance.ParameterSpace:

        strengths = [AnalogValueInterval(int(low), int(high))
                     for low, high in zip(lower_limits["strength"],
                                          upper_limits["strength"])]
        potentials = [ParameterIntervalDouble(float(low), float(high))
                      for low, high in zip(lower_limits["potential"],
                                           upper_limits["potential"])]
        OptionalDoubleInterval = grenade.MechanismSynapticInputConductance.\
            ParameterSpace.OptionalDoubleInterval
        references = [OptionalDoubleInterval(float(low), float(high))
                      for low, high in zip(lower_limits["reference"],
                                           upper_limits["reference"])]
        global_strengths = [
            AnalogValueInterval(int(low), int(high)) for low, high in
            zip(lower_limits["global_strength"],
                upper_limits["global_strength"])]

        time_constants = [TimeInterval(float(low), float(high))
                          for low, high in zip(lower_limits["time_constant"],
                                               upper_limits["time_constant"])]
        return grenade.MechanismSynapticInputConductance.ParameterSpace(
            i_synin_gm=strengths, synapse_dac_bias=global_strengths,
            e_reversal=potentials, e_reference=references,
            time_constant=time_constants)

    @classmethod
    def construct_parameterization(
        cls,
        values: Dict[str, Union[float, int]]
    ) -> grenade.MechanismSynapticInputConductance.Parameterization:
        return grenade.MechanismSynapticInputConductance.ParameterSpace.\
            Parameterization(
                i_synin_gm=[int(s) for s in values["strength"]],
                synapse_dac_bias=[int(s) for s in values["global_strength"]],
                e_reversal=[int(s) for s in values["potential"]],
                e_reference=[int(s) for s in values["reference"]],
                time_constant=[float(t) for t in values["time_constant"]])

    def copy(self) -> ConductanceBasedSynapse:
        '''
        Create copy of the mechanism.

        :return: Copy of the mechanism.
        '''
        syn = ConductanceBasedSynapse()
        syn.parameters = deepcopy(self.parameters)
        syn.receptor_type = copy(self.receptor_type)
        return syn
