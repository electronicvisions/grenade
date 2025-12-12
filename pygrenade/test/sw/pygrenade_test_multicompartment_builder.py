# !/usr/bin/env python
# pylint: disable=too-many-locals, too-many-statements
import unittest

from pygrenade_vx.network.multicompartment.tree import Node, Connection
from pygrenade_vx.network.multicompartment.neuron_components import Compartment
from pygrenade_vx.network.multicompartment.morphology_builder import \
    MorphologyBuilder


class SwTestPygrenadeVxMulticompartmentBuilder(unittest.TestCase):
    def test_tree(self):
        node_10 = Node({10}, label="node_10")
        node_20 = Node({20}, label="node_20")
        node_11 = Node({11}, label="node_11")
        node_22 = Node({22}, label="node_22")
        node_33 = Node({33}, label="node_33")
        node_a = Node({node_10, node_20},
                      [Connection(node_10, node_20)], label="node_a")
        node_b = Node({node_11, node_22, node_33},
                      [Connection(node_11, node_22),
                       Connection(node_22, node_33)], label="node_b")
        node_c = Node({node_a, node_b},
                      [Connection(node_10, node_11)], label="node_c")

        fully_labeled_leafs, _ = node_c.get_fully_labeled_leaf_elements()

        self.assertIn("node_c.node_a.node_10", fully_labeled_leafs.values())
        self.assertIn("node_c.node_a.node_20", fully_labeled_leafs.values())
        self.assertIn("node_c.node_b.node_11", fully_labeled_leafs.values())
        self.assertIn("node_c.node_b.node_22", fully_labeled_leafs.values())
        self.assertIn("node_c.node_b.node_33", fully_labeled_leafs.values())

        fully_labeled_connections = node_c.get_fully_labeled_connections(
            fully_labeled_leafs, True)

        for connection in fully_labeled_connections:
            self.assertIn(connection.source, fully_labeled_leafs.values())
            self.assertIn(connection.target, fully_labeled_leafs.values())

    def test_morphology_builder(self):
        builder = MorphologyBuilder()

        comp_a = builder.add_compartment(Compartment(), label="comp_a")
        comp_b = builder.add_compartment(Compartment(), label="comp_b")
        comp_c = builder.add_compartment(Compartment(), label="comp_b")
        comp_d = builder.add_compartment(Compartment(), label="comp_b")

        branch_a = builder.connect([Connection(comp_a, comp_b, 1)],
                                   label="branch_a")
        builder.connect([Connection(comp_b, comp_c, 1)], label="branch_b")
        builder.connect([Connection(comp_c, comp_d, 1)], label="branch_c")

        branch_d = builder.clone(branch_a, label="branch_d")

        comp_a_copy = builder.get_ref_in_node(branch_d, comp_a)

        branch_e = builder.connect([Connection(comp_a, comp_a_copy, 1)],
                                   label="branch_e")
        self.assertEqual(len(builder.nodes), 1)
        self.assertIn(branch_e, builder.nodes)

        _, labels = branch_e.get_fully_labeled_leaf_elements()
        self.assertIn("branch_e.branch_c.branch_b.branch_a.comp_a",
                      labels.keys())
        self.assertIn("branch_e.branch_c.branch_b.branch_a.comp_b",
                      labels.keys())
        self.assertIn("branch_e.branch_c.branch_b.comp_b",
                      labels.keys())
        self.assertIn("branch_e.branch_c.comp_b",
                      labels.keys())
        self.assertIn("branch_e.branch_d.comp_a",
                      labels.keys())
        self.assertIn("branch_e.branch_d.comp_b",
                      labels.keys())


if __name__ == "__main__":
    unittest.main()
