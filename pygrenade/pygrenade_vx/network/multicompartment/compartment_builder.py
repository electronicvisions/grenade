from pygrenade_vx.network.multicompartment.neuron_components import Compartment
from pygrenade_vx.network.multicompartment.mechanisms import Mechanism


class CompartmentBuilder:
    '''
    Construct a new compartment class.

    From this class compartments can be initialized which can be used to build
    a multi-compartmental neuron model.
    '''

    def __init__(self):
        self.mechanisms = {}

    def add(self, mechanism: Mechanism, label: str):
        '''
        Add the given mechansim to the compartment.

        :param mechanism: Mechanism to add.
        :param label: Label of the mechanism.
        '''
        if label in self.mechanisms:
            raise KeyError(f"Mechanism with label {label} "
                           "already exists on compartment.")
        self.mechanisms[label] = mechanism.copy()

    def done(self, name: str):
        '''
        Return a class which represents the constructed compartment.

        The class is derived from the `Compartment` class and mechansims
        are saved in a class member.

        :param name: Name of the created compartment class.

        :return: Built compartment class.
        '''
        new_compartment_class = type(name, (Compartment, ), {
            "_default_mechanisms": self.mechanisms
        })

        self.mechanisms = {}
        return new_compartment_class
