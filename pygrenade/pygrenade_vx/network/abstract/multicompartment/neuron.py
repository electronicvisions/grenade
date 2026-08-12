from copy import deepcopy
from typing import Any, Dict

import numpy as np

import _pygrenade_vx_network_abstract as grenade
from _pygrenade_vx_network_abstract import TimeInterval


class Neuron():

    '''
    Multicompartment-Neuron class.

    This is not the fully implemented neuron but contains the necessary data to
    construct a neuron in the final implementation from it.
    '''
    # The following parameters should be provided
    compartments = {}
    connections = []

    # parameters for parameter_space
    # (key = parameter_label, value = parameter_value)
    default_parameters = {}
    # translation of parameters (dummy: key = value)
    translations = {}

    # The following members are set in __init_subclass__
    compartment_ids_grenade = {}
    mechanisms_ids_grenade = {}
    connection_ids_grenade = {}

    def translate(self, parameters, copy=True):
        '''
        Return the neuron parameters.

        Overwritten to prevent a translation to itself to increase
        performance.

        :param parameters: Parameters of the neuron.
        :param copy: If true return a copy of the paramters.

        :return: Parameters of the neuron.
        '''
        # Copies the neuron parameters. No need to give tupels
        # with identical paramters.
        if copy:
            return deepcopy(parameters)
        return parameters

    def reverse_translate(self, native_parameters):
        '''
        Return the neuron parameters.

        Overwritten to prevent a translation to itself to increase
        performance.

        :param native_parameters: Parameters of the neuron.

        :return: Paramters of the neuron.
        '''
        # Copies the neuron parameters. No need to give tupels
        # with identical paramters.
        return native_parameters

    @classmethod
    def _compartments_to_grenade(cls) -> Dict[str, grenade.Compartment]:
        '''
        Convert the compartments of the neuron into grenade-compartments.

        :return: Dictionary mapping compartment-labels to
        grenade-compartments.
        '''
        compartments_grenade = {}
        for label, compartment in cls.compartments.items():
            compartments_grenade[label], mechanisms_mapping = \
                compartment.to_grenade()
            cls.mechanisms_ids_grenade[label] = mechanisms_mapping
        return compartments_grenade

    @staticmethod
    def _extract_parameters(parameters: Dict[str, Any],
                            mechanism_label: str,
                            mechanism):
        """
        Extract parameters which belong the given mechanism
        in the gvien compartment.

        :param parameters: Dictionary with full labels as keys.
        :param mechanism_label: Label for which to extract the parameters.
            The label should be the full label including the label of the
            mechanism itself.
        :param mechanism: Mechanism object for which to extract the parameters
        """
        param_names = mechanism.parameters.keys()
        params = {p_name: parameters[f"{mechanism_label}.{p_name}"] for p_name
                  in param_names}
        return params

    @classmethod
    def _construct_param_comp(
            cls,
            label: str,
            values: Dict[str, Any]):
        comp_param = grenade.Compartment.ParameterSpace.Parameterization()
        for mech_label, mech_id in cls.mechanisms_ids_grenade[label].items():
            mechanism = cls.compartments[label].mechanisms[mech_label]
            full_label = f"{label}.{mech_label}"
            mech_values = cls._extract_parameters(values,
                                                  full_label,
                                                  mechanism)
            mech_space = mechanism.construct_parameterization(mech_values)
            comp_param.mechanisms.set(mech_id, mech_space)  # pylint: disable=no-member
        return comp_param

    @classmethod
    def _construct_param_conn(
            cls,
            label: str,
            values: Dict[str, Any]):
        full_label = f"{label}.conductance"
        return grenade.CompartmentConnectionConductance.ParameterSpace.\
            Parameterization(np.array(values[full_label], dtype=float))

    @classmethod
    def construct_parameterization(cls,
                                   values: Dict[str, Any]):
        """
        Construct a parameterization with the given values.

        :param values: Dictionary with values. The key should be
            the full label to the different parameters.
        :return: Parameterization of the neuron.
        """
        neuron_param = grenade.Neuron.ParameterSpace.Parameterization()
        comp_spaces = {}
        for comp_label, comp_id in cls.compartment_ids_grenade.items():
            comp_param = cls._construct_param_comp(
                comp_label, values=values)
            comp_spaces[comp_id] = comp_param
        neuron_param.compartments = comp_spaces
        for conn_label, conn_id in cls.connection_ids_grenade.items():
            neuron_param.compartment_connections.set(  # pylint: disable=no-member
                conn_id,
                cls._construct_param_conn(conn_label, values))
        return neuron_param

    @classmethod
    def _construct_parameter_space_comp(
            cls,
            label: str,
            lower_limits: Dict[str, Any],
            upper_limits: Dict[str, Any]):
        comp_param = grenade.Compartment.ParameterSpace()
        for mech_label, mech_id in cls.mechanisms_ids_grenade[label].items():
            mechanism = cls.compartments[label].mechanisms[mech_label]
            full_label = f"{label}.{mech_label}"
            mech_lower = cls._extract_parameters(lower_limits,
                                                 full_label,
                                                 mechanism)
            mech_upper = cls._extract_parameters(upper_limits,
                                                 full_label,
                                                 mechanism)
            mech_space = mechanism.construct_parameter_space(mech_lower,
                                                             mech_upper)
            comp_param.mechanisms.set(mech_id, mech_space)  # pylint: disable=no-member
        return comp_param

    @classmethod
    def _construct_parameter_space_conn(
            cls,
            label: str,
            lower_limits: Dict[str, Any],
            upper_limits: Dict[str, Any]):
        full_label = f"{label}.conductance"
        intervals = [TimeInterval(float(low), float(high))
                     for low, high in zip(lower_limits[full_label],
                                          upper_limits[full_label])]
        return grenade.CompartmentConnectionConductance.ParameterSpace(
            intervals)

    @classmethod
    def construct_parameter_space(cls,
                                  lower_limits: Dict[str, Any],
                                  upper_limits: Dict[str, Any]):
        """
        Construct a parameter space with the given limits.

        :lower_limits: Dictionary with lower limits. The key should be
            the full label to the different parameters.
        :lower_limits: Dictionary with upper limits. The key should be
            the full label to the different parameters.
        :return: Parameter space with the given limits.
        """
        neuron_param = grenade.Neuron.ParameterSpace()
        compartments = {}
        for comp_label, comp_id in cls.compartment_ids_grenade.items():
            comp_param = cls._construct_parameter_space_comp(
                comp_label, lower_limits=lower_limits,
                upper_limits=upper_limits)
            compartments[comp_id] = comp_param
        neuron_param.compartments = compartments
        for conn_label, conn_id in cls.connection_ids_grenade.items():
            neuron_param.compartment_connections.set(  # pylint: disable=no-member
                conn_id,
                cls._construct_parameter_space_conn(
                    conn_label, lower_limits, upper_limits))
        return neuron_param

    @classmethod
    def _to_grenade(cls) -> grenade.Neuron:
        '''
        Build a grenade-neuron.

        :return: Grenade object of the neuron and its parameter space.
        '''
        neuron_grenade = grenade.Neuron()

        grenade_compartments = cls._compartments_to_grenade()

        # Add compartments to grenade neuron
        for label, grenade_compartment in grenade_compartments.items():
            compartment_id = neuron_grenade.add_compartment(
                grenade_compartment)
            cls.compartment_ids_grenade[label] = compartment_id

        # Add connections to grenade neuron
        for connection in cls.connections:
            compartment_id_source = cls.compartment_ids_grenade[
                connection.source]
            compartment_id_target = cls.compartment_ids_grenade[
                connection.target]
            conn_id = neuron_grenade.add_compartment_connection(
                compartment_id_source,
                compartment_id_target,
                grenade.CompartmentConnectionConductance())
            cls.connection_ids_grenade[connection.get_label()] = conn_id

        return neuron_grenade

    def __init_subclass__(cls):
        """
        Create grenade neuron.
        """
        # reset members which are dynamically inferred
        cls.compartment_ids_grenade = {}
        cls.mechanisms_ids_grenade = {}
        cls.connection_ids_grenade = {}

        cls.grenade_neuron = cls._to_grenade()
