# !/usr/bin/env python
import inspect
import unittest
from typing import Callable, Set, Type

from pygrenade_vx.network.abstract.multicompartment import mechanisms


class GenericMechanismTest(unittest.TestCase):
    @classmethod
    def generate_cases(cls):
        """
        Generate test cases for all implementations of
        :class:`mechanisms.Mechanism`.
        """
        for mechanism in cls.implementations(mechanisms.Mechanism):
            assert issubclass(mechanism, mechanisms.Mechanism)
            test_method = cls.generate_single(mechanism)
            test_method.__name__ = f"test_{mechanism.__name__}"

            setattr(cls, test_method.__name__, test_method)

    @staticmethod
    def generate_single(mechanism_type: Type[mechanisms.Mechanism],
                        ) -> Callable:
        """
        Generate a test function for testing the given mechanism.

        :param mechanism_type: Mechanism type to be tested.
        :return: Function testing a single run.
        """

        def test_func(self: GenericMechanismTest):
            try:
                mechanism = mechanism_type()
            except TypeError as error:
                self.skipTest(f"{mechanism_type.__name__} cannot be "
                              f"default-constructed: {error}")
                raise
            except ValueError as error:
                self.skipTest(f"{mechanism_type.__name__} cannot be "
                              f"default-constructed: {error}")
                raise

            n_instances = 10
            mock_parameterization = {name: [val] * n_instances for name, val
                                     in mechanism.parameters.items()}

            # Only test that functions are executable (do not test the output)
            mechanism.construct_parameterization(mock_parameterization)
            mechanism.construct_parameter_space(mock_parameterization,
                                                mock_parameterization)
            mechanism.copy()

        return test_func

    @staticmethod
    def implementations(class_: Type) -> Set[type]:
        """
        Recursively get all implementations/non-abstract children of a given
        parent/interface.

        :param class_: Parent class to be crawled for non-abstract subclasses.
        :return: Set of subclasses found.
        """
        ret = set()
        todo = {class_}

        while todo:
            current = todo.pop()
            todo.update(current.__subclasses__())

            if not inspect.isabstract(current):
                ret.add(current)

        return ret


GenericMechanismTest.generate_cases()

if __name__ == '__main__':
    unittest.main()
