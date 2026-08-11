from __future__ import annotations
from typing import Any, Optional, List, Dict, Tuple, Union


class Tree:
    '''
    Tree that contains children and connections between its children.
    '''
    def __init__(self,
                 children: List,
                 connections: Optional[List] = None,
                 label: Optional[str] = None):
        '''
        Tree that contains children and connections between its children.

        For leafs the child is not a Tree but a data class.

        :param children: Set of children.
        :param connections: Optional connections between children.
            if there is more than one child a connection is required.
        :param label: Optional label for the tree.
        '''
        self._validate_inputs(children, connections)

        self.label = label
        self.children = children
        self.connections = connections

    @classmethod
    def _validate_inputs(cls, children: List, connections: Optional[List]):
        if connections is None:
            connections = []

        if not cls._check_unique(children):
            raise ValueError("List of children is not unique.")
        if not cls._check_unique(connections):
            raise ValueError("List of connections is not unique.")

        if len(children) == 0:
            raise ValueError("You need to supplied at least one child")

        if not cls._valid_connections(children, connections):
            raise ValueError("The list of connections contains connections "
                             "between trees which are not part of any of "
                             "the provided children.")

        if not cls._acyclic_and_connected(children, connections):
            raise ValueError("Children and connections do not form a tree. "
                             "I.e. they are not acyclic and fully connected.")

    @staticmethod
    def _check_unique(values: List):
        return len(values) == len(set(values))

    @staticmethod
    def _contains_node(children: List, node: Union[Tree, Any]) -> bool:
        """
        Check whether `node` is one of `children` or nested inside one.
        """
        return any(
            child == node or (isinstance(child, Tree) and child.contains(node))
            for child in children
        )

    @classmethod
    def _valid_connections(cls, children: List, connections: List) -> bool:
        return all(
            cls._contains_node(children, connection.source)
            and cls._contains_node(children, connection.target)
            for connection in connections
        )

    @staticmethod
    def _get_owning_child(children: List, node: Union[Tree, Any]
                          ) -> Optional[Any]:
        """
        Return the top-level child that is, or contains, `node`.
        """
        for child in children:
            if child == node or (isinstance(child, Tree)
                                 and child.contains(node)):
                return child
        return None

    @classmethod
    def _acyclic_and_connected(cls,
                               children: List,
                               connections: List) -> bool:
        if len(connections) != len(children) - 1:
            return False

        groups = {child: i for i, child in enumerate(children)}

        for connection in connections:
            source = cls._get_owning_child(children, connection.source)
            target = cls._get_owning_child(children, connection.target)

            if source is None or target is None:
                return False

            if groups[source] == groups[target]:
                return False

            target_group = groups[target]
            for child, group in groups.items():
                if group == target_group:
                    groups[child] = groups[source]

        return True

    def get_fully_labeled_children(self) -> Tuple[Dict, Dict]:
        '''
        Return a mapping between the trees and their full labels.

        The full labels are created by concatenating the labels of all
        levels of the tree.

        :return: Two dictionaries containing the mapping from leafs to
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
        Return a mapping between the data classes stored in leafs
        and the full label of the leafs.

        The full labels are created by concatenating the labels of all
        levels of the tree.

        :return: Two dictionaries containing the mapping from the data in
            a leaf to the leaf label and the inverse mapping.
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
        Return connections with the full labels for connected trees.

        :return: List of connections with full tree labels.
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
            if isinstance(child, Tree):
                child_labeled_connections = child\
                    .get_fully_labeled_connections(full_labels, leaf_labels)
                if child_labeled_connections is not None:
                    connections_full_label.extend(child_labeled_connections)
        return connections_full_label

    def is_leaf(self) -> bool:
        '''
        Return whether a tree is a leaf i.e. only contains a single element
        of a data class.

        Assume that leafs do not contain other trees but a element
        of a data class.

        :return: Whether only a single element is contained.
        '''
        if len(self.children) == 1:
            # Is this always true??
            assert (not isinstance(next(iter(self.children)), Tree))
            return True
        return False

    def contains(self, tree) -> bool:
        '''
        Recursively iterates trough all children of the current tree
        to check whether the searched tree is within this tree.

        :return: Whether the searched for tree is part of this tree.
        '''
        if self == tree:
            return True
        if self.is_leaf():
            return False
        for child in self.children:
            if child == tree:
                return True
            if child.contains(tree):
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

    def get_label(self) -> str:
        """
        Generate a label for the current connection.

        The label is based on the labels of the source and target
        compartments.
        :return: Label which identifies this connection.
        """
        return f"connection.{self.source}...{self.target}"
