from typing import List, Optional, Tuple
from copy import deepcopy

from pygrenade_vx.network.multicompartment.neuron_components\
    import Compartment
from pygrenade_vx.network.multicompartment.neuron import\
    Neuron
from pygrenade_vx.network.multicompartment.tree import Node, Connection


class MorphologyBuilder:
    '''
    Construct a multicompartment neuron class.

    Compartments can be added and interconnected. The builder supports
    cloning branches of interconnected compartments and connecting different
    branches to one neuron. The created neuron must be fully connnected and
    acyclic.
    '''
    def __init__(self):
        '''
        Builder for multicompartment neuron classes.

        Structures the neuron during the building process in nodes that
        form a tree.
        '''
        self.nodes = []

    def add_compartment(self,
                        compartment: Compartment,
                        label: Optional[str] = None) -> Node:
        '''
        Add a compartment to the builder.

        :param compartment: Compartment to be added.
        :param label: Label of the compartment in the builder.
        :return: Node in the builder representing the compartment.
        '''
        node = Node([compartment], label=label)
        self.nodes.append(node)
        return node

    def connect(self,
                connections: List[Tuple[Node, Node]],
                label: Optional[str] = None) -> Node:
        '''
        Connect two compartments.

        By connecting two compartments a node that contains the two subtrees,
        that contain the source and target compartment of the connection is
        created.

        :param connections: List of source target tuples of compartments.
        :param label: Optional label for newly created subtree.
        :return: Newly created subtree containing the subtrees of the
        connected compartments.
        '''
        children = set()
        node_connections = set()
        for source, target in connections:
            # Find the top node containing the source compartment.
            source_root = None
            for node in self.nodes:
                if node.node_in_tree(source):
                    source_root = node
                    break
            # Find the top node containing the target compartment.
            target_root = None
            for node in self.nodes:
                if node.node_in_tree(target):
                    target_root = node
                    break

            if source_root is None or target_root is None:
                raise ValueError("Compartment not found in"
                                 " Morphology-Builder.")

            children.add(source_root)
            children.add(target_root)
            node_connections.add(Connection(source, target))

        # Creates new node and removes all nodes that are contained
        # in the new node.
        new_node = Node(children, node_connections, label)
        self.nodes.append(new_node)
        for child in children:
            self.nodes.remove(child)
        return new_node

    def clone(self,
              node: Node,
              label: Optional[str] = None) -> Node:
        '''
        Clone an existing node.

        :param node: Node to be cloned.
        :param label: Label of the clone.
        :return: Clone of the given node.
        '''
        new_node = deepcopy(node)
        new_node.label = label
        self.nodes.append(new_node)
        return new_node

    def get_ref_in_node(self,
                        target_tree: Node,
                        subnode: Node) -> Node:
        '''
        Return the reference to a node on a specific subtree.

        Intended for getting the reference of a node on a cloned subtree.
        :param target_tree: The subtree on which the node is searched.
        :param subnode: The node to be searched.
        :return: Reference of the node on the given subtree.
        '''
        full_label = None
        for node in self.nodes:
            if node == target_tree:
                continue

            labels, _ = node.get_fully_labeled_children()
            if subnode in labels:
                full_label = labels[subnode]
                break

        _, labels_target = target_tree.get_fully_labeled_children()
        for label_target, ref_target in labels_target.items():
            if label_target.split(".")[1:] == full_label.split(".")[
                    -len(label_target.split(".")) + 1:]:
                return ref_target
        return None

    def done(self,
             name: str) -> Neuron:
        '''
        Return a new neuron class that matches the topology defined in the
        builder.

        :param name: Name of the neuron class.
        :return: Neuron class.
        '''
        if len(self.nodes) != 1:
            raise ValueError("One root node needs to exist that"
                             " contains all other nodes. Otherwise the neuron"
                             " is not fully connected. Instead"
                             f" {len(self.nodes)} nodes exist.")

        # Get full labels for compartments and compartments in connections
        compartments_full_label, _ = self.nodes[0]\
            .get_fully_labeled_leaf_elements()
        connections_full_label = self.nodes[0]\
            .get_fully_labeled_connections(compartments_full_label)

        default_parameters = {}
        translations = {}

        for compartment, compartment_label in compartments_full_label.items():
            for mechanism_label, mechanism in compartment.mechanisms.items():
                for parameter_label, parameter in mechanism.parameters.items():
                    full_label = compartment_label + "." + mechanism_label\
                        + "." + parameter_label
                    default_parameters[full_label] =\
                        parameter
                    translations[full_label] = full_label

        new_neuron_class = type(name, (Neuron, ), {
            "compartments": deepcopy(compartments_full_label),
            "connections": deepcopy(connections_full_label),
            "default_parameters": default_parameters,
            "translations": translations})
        self.nodes = []
        return new_neuron_class
