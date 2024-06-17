from __future__ import annotations
from copy import deepcopy
import _pygrenade_vx_network as grenade


class Compartment:
    '''
    Substructure of multicompartment-neuron-model.
    '''
    _default_mechanisms = {}

    def __init__(self):
        '''
        Initialize compartment with mechanisms of compartments class.
        '''
        self.mechanisms = {}
        self.mechanisms = deepcopy(self._default_mechanisms)

    def to_grenade(self) -> grenade.Compartment:
        '''
        Create compartment in grenade.

        :return: Grenade object of the compartment.
        '''
        compartment_grenade = grenade.Compartment()
        for mechanism in self.mechanisms:
            compartment_grenade.add(mechanism.to_grenade())
        return compartment_grenade

    def copy(self) -> Compartment:
        '''
        Copy the compartment.

        :return: Copy of the compartment.
        '''
        new_compartment = deepcopy(self)
        return new_compartment


class CompartmentConnection:
    '''
    Connection between two compartments.
    '''
    def __init__(self, source: str, target: str,
                 conductance: int = 0):
        '''
        Initialize connection between source and target compartment
        with given conductance.

        :param source: Source compartment-descriptor of the connection.
        :param target: Target compartment-descriptor of the connection.
        :param conductance: Conductance of the connection.
        '''
        self.source = source
        self.target = target
        self.conductance = conductance
