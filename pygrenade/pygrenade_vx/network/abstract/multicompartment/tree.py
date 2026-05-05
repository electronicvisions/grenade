from typing import Optional, List, Dict, Tuple


class Node:
    '''
    Node that contains children and connecitons between its children.
    '''
    # TO-DO: enforce acyclic and connected??
    def __init__(self,
                 children: set,
                 connections: Optional[List] = None,
                 label: Optional[str] = None):
        '''
        Tree node that contains children and connections between its children.

        For leaf nodes the child is not a Node but a data class.

        :param children: Set of children.
        :param connections: Optional connections between children.
            if there is more than one child a connection is required.
        :param label: Optional label for the node.
        '''
        self.label = label
        self.children = children
        self.connections = connections
        # Check node validity. Only leaf nodes do not need connections
        if (len(self.children) > 1) and self.connections is None:
            raise ValueError("More than two children are in this node but"
                             " no connection is defined.")
        if (len(self.children) > 1) and len(self.connections) <\
                len(self.children) - 1:
            raise ValueError(f"A node with {len(self.children)} children"
                             f" requires {len(self.children) - 1}"
                             f" connections to be"
                             f" connected and acyclic. Instead"
                             f" {len(self.connections)} connections were"
                             f" provided.")

    def get_fully_labeled_children(self) -> Tuple[Dict, Dict]:
        '''
        Return a mapping between the nodes and their full labels.

        The full labels are created by concatenating the labels of all
        levels of the tree.

        :return: Two dicitonaries containing the mapping from leaf nodes to
            their labels and the inverse mapping.
        '''
        labels = {}
        label_layer_self = ""
        if self.label is not None:
            label_layer_self = self.label + "."

        for child in self.children:
            if child.is_leaf():
                labels[child] = label_layer_self + child.label
            else:
                child_labels, _ = child.get_fully_labeled_children()
                for child, child_label in child_labels.items():
                    labels[child] = label_layer_self + child_label
        inverse_labels = {v: k for k, v in labels.items()}
        return labels, inverse_labels

    def get_fully_labeled_leaf_elements(self) -> Tuple[Dict, Dict]:
        '''
        Return a mapping between the data classes stored in leaf nodes
        and the full label of the leaf nodes.

        The full labels are created by concatenating the labels of all
        levels of the tree.

        :return: Two dictionaries containing the mapping from the data in
            a leaf to the leaf node label and the inverse mapping.
        '''
        fully_labeled_children, _ = self.get_fully_labeled_children()
        fully_labeled_leaf_elements = {}
        for child, child_label in fully_labeled_children.items():
            fully_labeled_leaf_elements[next(iter(child.children))]\
                = child_label

        inverse_leaf_element_labels = {v: k for k, v
                                       in fully_labeled_leaf_elements.items()}
        return fully_labeled_leaf_elements, inverse_leaf_element_labels

    def get_fully_labeled_connections(self,
                                      full_labels: Dict,
                                      leaf_labels: bool = True) -> List:
        '''
        Return connections with the full labels for connected nodes.

        :return: List of connections with full node labels.
        '''
        connections_full_label = []
        if self.connections is None:
            return None
        for connection in self.connections:
            if leaf_labels:
                connections_full_label.append(Connection(
                    full_labels[next(iter(connection.source.children))],
                    full_labels[next(iter(connection.target.children))],
                    connection.strength))
            else:
                connections_full_label.append(Connection(
                    full_labels[connection.source],
                    full_labels[connection.target],
                    connection.strength))

        for child in self.children:
            if isinstance(child, Node):
                child_labeled_connections = child\
                    .get_fully_labeled_connections(full_labels, leaf_labels)
                if child_labeled_connections is not None:
                    connections_full_label.extend(child_labeled_connections)
        return connections_full_label

    def is_leaf(self) -> bool:
        '''
        Return wether a node is a leaf i.e. only contains a single element
        of a data class.

        Assume that a leaf node does not contain another node but a element
        of a data class.

        :return: Wether only a single element is contained.
        '''
        if len(self.children) == 1:
            # Is this always true??
            assert (not isinstance(next(iter(self.children)), Node))
            return True
        return False

    def node_in_tree(self,
                     node) -> bool:
        '''
        Recursively iterates trough all children of the current node
        to check wether the searched node is within the tree of this node.

        :return: Wether the searched for node is in the tree of this node.
        '''
        if self == node:
            return True
        if self.is_leaf():
            return False
        for child in self.children:
            if child == node:
                return True
            if child.node_in_tree(node):
                return True
        return False


class Connection:
    '''
    Connection that contains source and target of a connection.
    '''
    def __init__(self,
                 source,
                 target,
                 strength=None):
        self.source = source
        self.target = target
        self.strength = strength
