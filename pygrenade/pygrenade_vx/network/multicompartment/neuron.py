from copy import deepcopy
from typing import Dict
import _pygrenade_vx_network as grenade


class Neuron():

    '''
    Multicompartment-Neuron class.

    This is not the fully implemented neuron but contains the necessary data to
    construct a neuron in the final implementation from it.
    '''
    compartments = {}
    compartment_ids_grenade = {}
    connections = []

    # parameters for parameter_space
    # (key = parameter_label, value = parameter_value)
    default_parameters = {}
    # translation of parameters (dummy: key = value)
    translations = {}

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

    def _compartments_to_grenade(self) -> Dict[str, grenade.Compartment]:
        '''
        Convert the compartments of the neuron into grenade-compartments.

        :return: Dictionary mapping compartment-labels to
        grenade-compartments.
        '''
        compartments_grenade = {}
        for label, compartment in self.compartments.items():
            compartments_grenade[label] = compartment.to_grenade()
        return compartments_grenade

    def neuron_to_grenade(self) -> grenade.Neuron:
        '''
        Build a grenade-neuron and adds compartments
        and compartment-connections to grenade-neuron.

        :return: Grenade object of the neuron.
        '''
        neuron_grenade = grenade.Neuron()

        # Add compartments to grenade neuron
        for label, compartment_grenade in self._compartments_to_grenade()\
                .items():
            compartment_id = neuron_grenade.add_compartment(
                compartment_grenade)
            self.compartment_ids_grenade[label] = compartment_id

        # Add connections to grenade neuron
        for connection in self.connections:
            compartment_id_source = self.compartment_ids_grenade[
                connection.source]
            compartment_id_target = self.compartment_ids_grenade[
                connection.target]
            conductance = connection.conductance
            connection_grenade = grenade.CompartmentConnectionConductance(
                conductance)
            neuron_grenade.add_compartment_connection(
                compartment_id_source,
                compartment_id_target,
                connection_grenade)
        return neuron_grenade
