from typing import List, Optional
from copy import deepcopy

from pygrenade_vx.network.abstract.multicompartment.neuron_components\
    import Compartment
from pygrenade_vx.network.abstract.multicompartment.neuron import\
    Neuron
from pygrenade_vx.network.abstract.multicompartment.tree import\
    Tree, Connection


class MorphologyBuilder:
    '''
    Construct a multicompartment neuron class.

    :param neuron_type: Base class from which the neuron created in the builder
    derives.

    Compartments can be added and interconnected. The builder supports
    cloning branches of interconnected compartments and connecting different
    branches to one neuron. The created neuron must be fully connnected and
    acyclic.
    '''
    def __init__(self,
                 neuron_type=Neuron):
        '''
        Builder for multicompartment neuron classes.

        Structures the neuron during the building process in subtrees that
        form a tree.
        '''
        self.trees = []
        self.neuron_type = neuron_type

    def add_compartment(self,
                        compartment: Compartment,
                        label: str) -> Tree:
        '''
        Add a compartment to the builder.

        :param compartment: Compartment to be added.
        :param label: Label of the compartment in the builder.
        :return: Tree in the builder representing the compartment.
        '''
        tree = Tree([deepcopy(compartment)], label=label)
        self.trees.append(tree)
        return tree

    def connect(self,
                connections: List[Connection],
                label: Optional[str] = None) -> Tree:
        '''
        Connect two compartments.

        By connecting two compartments a tree that contains the two subtrees,
        that contain the source and target compartment of the connection is
        created.

        :param connections: List of connections.
        :param conductances: List of conductances for connections.
        :param label: Optional label for newly created subtree.
        :return: Newly created subtree containing the subtrees of the
        connected compartments.
        '''
        # Set does not preserve order -> use dict to save list of unique
        # elements in keys
        children = {}
        tree_connections = {}
        for connection in connections:
            # Find the top tree containing the source compartment.
            source_root = None
            for tree in self.trees:
                if tree.contains(connection.source):
                    source_root = tree
                    break
            # Find the top tree containing the target compartment.
            target_root = None
            for tree in self.trees:
                if tree.contains(connection.target):
                    target_root = tree
                    break

            if source_root is None or target_root is None:
                raise ValueError("Compartment not found in"
                                 " Morphology-Builder.")

            children[source_root] = None
            children[target_root] = None
            tree_connections[connection] = None

        children = list(children.keys())
        tree_connections = list(tree_connections.keys())

        # Creates new tree and removes all trees that are contained
        # in the new tree.
        new_tree = Tree(children, tree_connections, label)
        self.trees.append(new_tree)
        for child in children:
            self.trees.remove(child)
        return new_tree

    def clone(self,
              tree: Tree,
              label: Optional[str] = None) -> Tree:
        '''
        Clone an existing tree.

        :param tree: Tree to be cloned.
        :param label: Label of the clone.
        :return: Clone of the given tree.
        '''
        new_tree = deepcopy(tree)
        new_tree.label = label
        self.trees.append(new_tree)
        return new_tree

    def get_ref_in_tree(self,
                        target_tree: Tree,
                        subtree: Tree) -> Tree:
        '''
        Return the reference to a tree on a specific subtree.

        Intended for getting the reference of a tree on a cloned subtree.
        :param target_tree: The subtree on which the tree is searched.
        :param subtree: The tree to be searched.
        :return: Reference of the tree on the given subtree.
        '''
        full_label = None
        for tree in self.trees:
            if tree == target_tree:
                continue

            labels, _ = tree.get_fully_labeled_children()
            if subtree in labels:
                full_label = labels[subtree]
                break

        _, labels_target = target_tree.get_fully_labeled_children()
        for label_target, ref_target in labels_target.items():
            if label_target.split(".")[1:] == full_label.split(".")[
                    -len(label_target.split(".")) + 1:]:
                return ref_target
        return None

    def build_defaults(self,
                       compartments_full_label,
                       connections_full_label):
        default_parameters = {}
        for compartment_label, compartment in compartments_full_label.items():
            for mechanism_label, mechanism in compartment.mechanisms.items():
                for parameter_label, parameter in mechanism.parameters.items():
                    full_label = compartment_label + "." + mechanism_label\
                        + "." + parameter_label
                    default_parameters[full_label] =\
                        parameter

        for connection in connections_full_label:
            param_label = connection.get_label() + ".conductance"
            default_parameters[param_label] = connection.strength

        return default_parameters

    def done(self,
             name: str):
        '''
        Return a new neuron class that matches the topology defined in the
        builder.

        :param name: Name of the neuron class.
        :return: Neuron class.
        '''
        if len(self.trees) != 1:
            raise ValueError("One root tree needs to exist that"
                             " contains all other trees. Otherwise the neuron"
                             " is not fully connected. Instead"
                             f" {len(self.trees)} trees exist.")

        # Get full labels for compartments and compartments in connections
        compartments_full_label_inverse, compartments_full_label = self\
            .trees[0].get_fully_labeled_leaf_elements()
        connections_full_label = self.trees[0]\
            .get_fully_labeled_connections(compartments_full_label_inverse)

        default_parameters = self.build_defaults(
            compartments_full_label, connections_full_label)

        new_neuron_class = type(name, (self.neuron_type, ), {
            "compartments": deepcopy(compartments_full_label),
            "connections": deepcopy(connections_full_label),
            "default_parameters": default_parameters})
        self.trees = []
        return new_neuron_class
