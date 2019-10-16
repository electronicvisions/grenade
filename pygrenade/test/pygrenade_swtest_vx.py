#!/usr/bin/env python
import unittest
from dlens_vx import lola, hal, halco
import pygrenade_vx as grenade


class SwTestPygrenadeVx(unittest.TestCase):
    # pylint: disable=no-member,unsubscriptable-object
    def test_hemisphere_config(self):
        config = grenade.HemisphereConfig()
        self.assertIsInstance(config.synapse_matrix, lola.SynapseMatrix)
        self.assertIsInstance(
            config.synapse_driver_block[
                halco.SynapseDriverOnSynapseDriverBlock()],
            hal.SynapseDriverConfig)

        other = grenade.HemisphereConfig()
        self.assertEqual(config, other)

        other.synapse_matrix.weights[0][0] = 15
        self.assertNotEqual(config, other)

    def test_chip_config(self):
        config = grenade.ChipConfig()
        self.assertIsInstance(config.hemispheres[0], grenade.HemisphereConfig)

        other = grenade.ChipConfig()
        self.assertEqual(config, other)

        other.hemispheres[0].synapse_matrix.weights[0][0] = 15
        self.assertNotEqual(config, other)


if __name__ == "__main__":
    unittest.main()
