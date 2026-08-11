# !/usr/bin/env python
# pylint: disable=too-many-locals, too-many-statements
import unittest

from pygrenade_vx.network.abstract.multicompartment.mechanisms import \
    CurrentBasedSynapse, MembraneCapacitance
from pygrenade_vx.network.abstract.multicompartment.compartment_builder \
    import CompartmentBuilder
from pygrenade_vx.network.abstract.multicompartment.tree import \
    Tree, Connection
from pygrenade_vx.network.abstract.multicompartment.neuron_components import \
    Compartment
from pygrenade_vx.network.abstract.multicompartment.morphology_builder import \
    MorphologyBuilder


class SwTestPygrenadeVxMulticompartmentBuilder(unittest.TestCase):
    def test_tree(self):
        tree_10 = Tree([10], label="tree_10")
        tree_20 = Tree([20], label="tree_20")
        tree_11 = Tree([11], label="tree_11")
        tree_22 = Tree([22], label="tree_22")
        tree_33 = Tree([33], label="tree_33")
        tree_a = Tree([tree_10, tree_20],
                      [Connection(tree_10, tree_20)], label="tree_a")
        tree_b = Tree({tree_11, tree_22, tree_33},
                      [Connection(tree_11, tree_22),
                       Connection(tree_22, tree_33)], label="tree_b")
        tree_c = Tree({tree_a, tree_b},
                      [Connection(tree_10, tree_11)], label="tree_c")

        fully_labeled_leafs, _ = tree_c.get_fully_labeled_leaf_elements()

        self.assertIn("tree_c.tree_a.tree_10", fully_labeled_leafs.values())
        self.assertIn("tree_c.tree_a.tree_20", fully_labeled_leafs.values())
        self.assertIn("tree_c.tree_b.tree_11", fully_labeled_leafs.values())
        self.assertIn("tree_c.tree_b.tree_22", fully_labeled_leafs.values())
        self.assertIn("tree_c.tree_b.tree_33", fully_labeled_leafs.values())

        fully_labeled_connections = tree_c.get_fully_labeled_connections(
            fully_labeled_leafs, True)

        for connection in fully_labeled_connections:
            self.assertIn(connection.source, fully_labeled_leafs.values())
            self.assertIn(connection.target, fully_labeled_leafs.values())

        # Invalid connection
        with self.assertRaises(ValueError):
            Tree([tree_10, tree_20], [Connection(tree_10, tree_11)])
        # Cycles
        with self.assertRaises(ValueError):
            Tree([tree_10, tree_20], [Connection(tree_10, tree_10)])
        with self.assertRaises(ValueError):
            Tree([tree_10, tree_20], [Connection(tree_10, tree_20),
                                      Connection(tree_20, tree_10)])

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

        comp_a_copy = builder.get_ref_in_tree(branch_d, comp_a)

        branch_e = builder.connect([Connection(comp_a, comp_a_copy, 1)],
                                   label="branch_e")
        self.assertEqual(len(builder.trees), 1)
        self.assertIn(branch_e, builder.trees)

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

    def test_mapping_to_grenade_neuron(self):

        # test with mechanisms
        c_builder = CompartmentBuilder()
        c_builder.add(CurrentBasedSynapse(), label="syn")
        c_builder.add(MembraneCapacitance(), label="cap")
        MyCompartment = c_builder.done("MyCompartment")

        builder = MorphologyBuilder()

        comp_a = builder.add_compartment(Compartment(), label="comp_a")
        comp_b = builder.add_compartment(MyCompartment(), label="comp_b")

        builder.connect([Connection(comp_a, comp_b, 1)], label="branch_a")

        MyNeuron = builder.done("MyNeuron")
        my_neuron = MyNeuron()

        my_neuron._to_grenade()  # pylint: disable=protected-access


if __name__ == "__main__":
    unittest.main()
