from abc import abstractmethod
from typing import Dict, List, Optional
import time
import pygrenade_vx.execution
import pygrenade_vx.common
import pygrenade_vx.signal_flow
import _pygrenade_vx_network_abstract
import pygrenade_common
from dlens_vx_v3 import logger


class ExperimentElement:
    """
    Element of experiment description, e.g. Population, Projection,
    Recorder.

    :ivar changed_topology: Whether changes occured since last adding the
                            element to the experiment's topology, which
                            require calling add_to_topology to update the
                            topology representation.
    :ivar changed_input_data: Whether changes occured since last adding the
                              element to the experiment's input data, which
                              require calling add_to_input_data to update
                              the input data representation.
    """
    def __init__(self, experiment: "Experiment"):
        """
        Construct element and add it to the experiment.

        :param experiment: Experiment to add element to
        """
        experiment.elements.append(self)
        self.changed_topology: bool = True
        self.changed_input_data: bool = True

    def add_to_topology(
            self,
            experiment: "ExperimentSnippet") -> bool:
        """
        Add element to experiment snippet's topology.

        If the snippet doesn't contain the element yet, it is added.
        If it is already contained it is updated, if changed_topology
        is True.
        This method is called before the potential mapping operation.

        :param experiment: Experiment snippet for which to add this element
                           into the snippet's topology
        :return: Whether the element was added to the topology. The
                 experiment doesn't track interdependencies defining the
                 order in which the elements can be added to the topology.
                 Therefore this method is expected to perform the check
                 whether the element can be added or updated, in which case
                 the return value is expected to be True.
        """
        raise NotImplementedError()

    def add_to_input_data(
            self,
            experiment: "ExperimentSnippet",
            snippet_begin_time,
            snippet_end_time):
        """
        Add element to experiment snippet's input data.

        If the snippet doesn't contain the element yet, it is added.
        If it is already contained it is updated, if changed_input_data
        is True.
        The method is called after mapping is available.

        :param experiment: Experiment snippet for which to add this element
                           into the snippet's input data
        :param snippet_begin_time: Time at which the snippet begins
        :param snippet_end_time: Time at which the snippet ends
        """
        raise NotImplementedError()

    def extract_output_data(self, experiment: List["ExperimentSnippet"])\
            -> None:
        """
        Extract output data of element from experiment snippet.
        This method is called after execution of the experiment.
        Storage in the front end format is to be performed by the front end.

        :param experiment: Experiment snippet from which to extract this
                           element's output data
        """
        raise NotImplementedError()


class ExperimentSnippet:
    """
    Experiment description of a realtime snippet.
    The snippet contains the experiment elements as well as the model and
    the back end grenade representation.
    """
    def __init__(self):
        # experiment elements
        self.elements: List[ExperimentElement] = []

        # grenade model representation
        self.topology = pygrenade_common.Topology()
        self.input_data = pygrenade_common.InputData()
        self.output_data: Optional[pygrenade_common.OutputData] = None

        # grenade back end representation
        self.mapped_topology = None
        self.mapped_input_data = None

        self.last_topology = None
        self.last_input_data = None

    def copy(self):
        snippet = ExperimentSnippet()

        snippet.elements = list(e for e in self.elements)

        mapped_topology = self.mapped_topology.deepcopy()
        snippet.mapped_topology = mapped_topology

        snippet.topology = mapped_topology.get_root()

        input_data = pygrenade_common.InputData(
            self.input_data)
        output_data = None
        if self.output_data:
            output_data = pygrenade_common.OutputData(
                self.output_data)

        snippet.input_data = input_data
        snippet.output_data = output_data

        if self.last_topology:
            snippet.last_topology = pygrenade_common.Topology(
                self.last_topology)

        if self.last_input_data:
            assert self.last_input_data == self.input_data
            snippet.last_input_data = pygrenade_common.InputData(
                snippet.input_data)

        return snippet


class Experiment:
    """
    Stateful experiment description.
    """
    def __init__(
            self,
            calibration: Optional[
                _pygrenade_vx_network_abstract.Calibration] = None,
            mapper: Optional[
                _pygrenade_vx_network_abstract.Mapper] = None):
        if calibration is not None:
            self.calibration = calibration
        else:
            self.calibration = _pygrenade_vx_network_abstract.JITCalibration()

        if mapper is not None:
            self.mapper = mapper
        else:
            self.mapper = _pygrenade_vx_network_abstract.GreedyMapper()

        self.snippets: List[ExperimentSnippet] = [ExperimentSnippet()]
        self.elements: List[ExperimentElement] = []

        self.hooks = {
            pygrenade_common.ExecutionInstanceOnExecutor():
            pygrenade_vx.execution.ExecutionInstanceHooks()}

        self.log = logger.get("grenade.network.abstract.Experiment")

    @abstractmethod
    def generate_runtimes(self, runtime) \
            -> Dict[
                pygrenade_common.TimeDomainOnTopology,
                pygrenade_common.TimeDomainRuntimes]:
        """
        Generate runtimes for current snippet.
        This method is called during filling a snippet.
        """
        raise NotImplementedError()

    def fill_snippet(
            self,
            snippet_begin_time,
            snippet_end_time,
            executor: pygrenade_vx.execution.JITGraphExecutorHandle):
        """
        Fill current snippet by adding its elements, creating a mapping,
        adding the elements' input data and mapping
        topology and input data.
        """
        snippet = self.snippets[-1]

        snippet.elements = list(self.elements)
        not_added_elements = list(snippet.elements)
        while not_added_elements:
            any_success = False
            any_changed = False
            for element in not_added_elements.copy():
                if not element.changed_topology:
                    not_added_elements.remove(element)
                    continue
                any_changed = True
                begin = time.time()
                success = element.add_to_topology(
                    snippet)
                self.log.DEBUG(f"fill_snippet(): Added {element} to topology "
                               f"in {(time.time() - begin):.3f}s")
                if success:
                    not_added_elements.remove(element)
                    any_success = True
                    element.changed_topology = False
            if any_changed and not any_success:
                raise RuntimeError(
                    "No element added to experiment, loop terminated.")

        begin = time.time()
        snippet = self.snippets[-1]
        self._map_snippet(snippet, executor)
        self.log.DEBUG(
            f"fill_snippet(): Mapped in {(time.time() - begin):.3f}s")

        for element in snippet.elements:
            if not element.changed_input_data:
                continue
            begin = time.time()
            element.add_to_input_data(
                snippet, snippet_begin_time, snippet_end_time)
            element.changed_input_data = False
            self.log.DEBUG(
                f"fill_snippet(): Added {element} to input data "
                f"in {(time.time() - begin):.3f}s")

        for time_domain, runtimes in self.generate_runtimes(
                snippet_end_time - snippet_begin_time).items():
            snippet.input_data.time_domain_runtimes.set(
                time_domain, runtimes)

        begin = time.time()
        self._map_snippet_input_data(snippet)
        if snippet.last_input_data is None \
                or snippet.last_input_data != \
                snippet.input_data:
            snippet.last_input_data = \
                pygrenade_common.InputData(snippet.input_data)
        self.log.DEBUG(
            f"fill_snippet(): Mapped input data "
            f"in {(time.time() - begin):.3f}s")

    def add_snippet(
            self,
            snippet_begin_time,
            snippet_end_time,
            executor: pygrenade_vx.execution.JITGraphExecutorHandle):
        """
        Add fill current snippet and add a new one.
        """
        self.fill_snippet(
            snippet_begin_time, snippet_end_time, executor)

        begin = time.time()
        snippet = self.snippets[-1].copy()
        self.log.DEBUG(
            f"add_snippet(): Copied snippet "
            f"in {(time.time() - begin):.3f}s")

        # TODO: is this needed, since we do it also in fill_snippet above?
        begin = time.time()
        snippet.mapped_input_data = \
            snippet.mapped_topology.map_root_input_data(
                snippet.input_data)
        self.log.DEBUG(
            f"add_snippet(): Created mapped snippet "
            f"in {(time.time() - begin):.3f}s")

        self.snippets.append(snippet)

    def run(self, executor: pygrenade_vx.execution.JITGraphExecutorHandle):
        """
        Execute all currently contained snippets with given executor.
        Distribute output data to elements of snippets.
        """
        # map input data of last snippet again in order to support users
        # changing actual hardware parameters in mapping object
        self._map_snippet_input_data(self.snippets[-2])

        output_data = pygrenade_vx.execution.run(
            executor,
            [s.mapped_topology for s in self.snippets[:-1]],
            [s.mapped_input_data for s in self.snippets[:-1]],
            self.hooks)

        for i, snippet in enumerate(self.snippets[:-1]):
            snippet.output_data = self.snippets[i].mapped_topology\
                .map_root_output_data(output_data[i])

        self._distribute_output_data()

    def reset(self):
        """
        Reset experiment by removing all contained snippets.
        """
        self.snippets = [self.snippets[-1]]

    def _map_snippet(
            self, snippet: ExperimentSnippet,
            executor: pygrenade_vx.execution.JITGraphExecutorHandle):
        if not snippet.last_topology or snippet.last_topology != \
                snippet.topology:
            snippet.mapped_topology = self.mapper(
                snippet.topology,
                self.calibration,
                executor)
            snippet.last_topology = pygrenade_common.Topology(
                snippet.topology)

    def _map_snippet_input_data(self, snippet: ExperimentSnippet):
        snippet.mapped_input_data = \
            snippet.mapped_topology.map_root_input_data(
                snippet.input_data)

    def _distribute_output_data(self):
        """
        Distribute post-execution output data to snippet elements.
        """
        for snippet in self.snippets[:-1]:
            if snippet.output_data is None:
                raise RuntimeError(
                    "Output data is only available after execution.")
        for element in self.elements:
            element.extract_output_data(self.snippets[:-1])
