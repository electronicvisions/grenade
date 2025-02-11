#include "grenade/vx/network/abstract/plasticity_rule.h"

#include "grenade/common/execution_instance_id.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/multi_index_sequence/cuboid.h"
#include "grenade/common/time_domain_on_topology.h"
#include "grenade/common/time_domain_runtimes.h"
#include "grenade/vx/network/abstract/clock_cycle_time_domain_runtimes.h"
#include "grenade/vx/network/abstract/vertex_port_type/analog_observable.h"
#include "grenade/vx/network/abstract/vertex_port_type/synapse_observable.h"
#include "grenade/vx/signal_flow/types.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/variant.h"
#include <memory>
#include <stdexcept>

namespace grenade::vx::network::abstract {

PlasticityRule::Parameterization::Parameterization(std::string kernel) : kernel(kernel) {}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Parameterization::copy() const
{
	return std::make_unique<PlasticityRule::Parameterization>(*this);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Parameterization::move()
{
	return std::make_unique<PlasticityRule::Parameterization>(std::move(*this));
}

bool PlasticityRule::Parameterization::is_equal_to(grenade::common::PortData const& other) const
{
	return kernel == static_cast<PlasticityRule::Parameterization const&>(other).kernel;
}

std::ostream& PlasticityRule::Parameterization::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "Parameterization(\n";
	ios << hate::Indentation("\t");
	ios << kernel << "\n";
	ios << hate::Indentation() << ")";
	return os;
}


PlasticityRule::Dynamics::Dynamics(Timer const& timer, size_t batch_size) :
    timer(timer), m_batch_size(batch_size)
{
}

size_t PlasticityRule::Dynamics::batch_size() const
{
	return m_batch_size;
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Dynamics::copy() const
{
	return std::make_unique<PlasticityRule::Dynamics>(*this);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Dynamics::move()
{
	return std::make_unique<PlasticityRule::Dynamics>(std::move(*this));
}

bool PlasticityRule::Dynamics::is_equal_to(grenade::common::PortData const& other) const
{
	auto const& other_dyn = static_cast<PlasticityRule::Dynamics const&>(other);
	return timer == other_dyn.timer && m_batch_size == other_dyn.m_batch_size;
}

std::ostream& PlasticityRule::Dynamics::print(std::ostream& os) const
{
	return os << "Dynamics(" << timer << ", " << m_batch_size << ")";
}


std::ostream& operator<<(std::ostream& os, PlasticityRule::Results::TimedData const& value)
{
	hate::IndentingOstream ios(os);
	ios << "TimedData(\n";
	ios << hate::Indentation("\t");
	ios << "data_per_synapse:\n";
	for (auto const& [name, data] : value.data_per_synapse) {
		ios << hate::Indentation("\t\t");
		ios << name << ":\n";
		for (auto const& [projection, d] : data) {
			ios << hate::Indentation("\t\t\t");
			ios << projection << ":\n";
			std::visit(
			    [&ios](auto const& d) {
				    for (size_t b = 0; auto const& batch_entry : d) {
					    ios << hate::Indentation("\t\t\t\t");
					    ios << "batch entry " << b << ":\n";
					    for (auto const& timed_data : batch_entry) {
						    ios << hate::Indentation("\t\t\t\t\t");
						    ios << timed_data.time << ":\n";
						    ios << hate::Indentation("\t\t\t\t\t\t");
						    for (auto const& row : timed_data.data) {
							    ios << hate::join(row, ", ") << "\n";
						    }
					    }
					    b++;
				    }
			    },
			    d);
		}
	}
	ios << hate::Indentation("\t");
	ios << "data_per_neuron:\n";
	for (auto const& [name, data] : value.data_per_neuron) {
		ios << hate::Indentation("\t\t");
		ios << name << ":\n";
		for (auto const& [population, d] : data) {
			ios << hate::Indentation("\t\t\t");
			ios << population << ":\n";
			std::visit(
			    [&ios](auto const& d) {
				    for (size_t b = 0; auto const& batch_entry : d) {
					    ios << hate::Indentation("\t\t\t\t");
					    ios << "batch entry " << b << ":\n";
					    for (auto const& timed_data : batch_entry) {
						    ios << hate::Indentation("\t\t\t\t\t");
						    ios << timed_data.time << ":\n";
						    ios << hate::Indentation("\t\t\t\t\t\t");
						    for (size_t n = 0; auto const& neuron : timed_data.data) {
							    ios << "neuron " << n << ":\n";
							    ios << hate::Indentation("\t\t\t\t\t\t\t");
							    for (auto const& [compartment, compartment_data] : neuron) {
								    ios << compartment << ": " << hate::join(compartment_data, ", ")
								        << "\n";
							    }
							    n++;
						    }
					    }
					    b++;
				    }
			    },
			    d);
		}
	}
	ios << hate::Indentation("\t");
	ios << "data_array:\n";
	for (auto const& [name, data] : value.data_array) {
		ios << hate::Indentation("\t\t");
		ios << name << ":\n";
		std::visit(
		    [&ios](auto const& d) {
			    for (size_t b = 0; auto const& batch_entry : d) {
				    ios << hate::Indentation("\t\t\t");
				    ios << "batch entry " << b << ":\n";
				    for (auto const& timed_data : batch_entry) {
					    ios << hate::Indentation("\t\t\t\t");
					    ios << timed_data.time << ": " << hate::join(timed_data.data, ", ") << "\n";
				    }
				    b++;
			    }
		    },
		    data);
	}
	return os;
}


PlasticityRule::Results::Results(Data data) : data(std::move(data)) {}

size_t PlasticityRule::Results::batch_size() const
{
	return std::visit(
	    hate::overloaded{
	        [](grenade::vx::network::abstract::PlasticityRule::Results::RawData const& d) {
		        return d.data.size();
	        },
	        [](auto const& d) -> size_t {
		        if (!d.data_per_synapse.empty()) {
			        auto const& per_projection = d.data_per_synapse.begin()->second;
			        if (!per_projection.empty()) {
				        return std::visit(
				            [](auto const& dd) { return dd.size(); },
				            per_projection.begin()->second);
			        }
		        }
		        if (!d.data_per_neuron.empty()) {
			        auto const& per_projection = d.data_per_neuron.begin()->second;
			        if (!per_projection.empty()) {
				        return std::visit(
				            [](auto const& dd) { return dd.size(); },
				            per_projection.begin()->second);
			        }
		        }
		        if (!d.data_array.empty()) {
			        return std::visit(
			            [](auto const& dd) { return dd.size(); }, d.data_array.begin()->second);
		        }
		        return 0;
	        }},
	    data);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Results::copy() const
{
	return std::make_unique<PlasticityRule::Results>(*this);
}

std::unique_ptr<grenade::common::PortData> PlasticityRule::Results::move()
{
	return std::make_unique<PlasticityRule::Results>(std::move(*this));
}

bool PlasticityRule::Results::is_equal_to(grenade::common::PortData const& other) const
{
	return data == static_cast<Results const&>(other).data;
}

std::ostream& PlasticityRule::Results::print(std::ostream& os) const
{
	std::visit(
	    [&os](auto const& d) {
		    hate::IndentingOstream ios(os);
		    ios << "Results(\n";
		    ios << hate::Indentation("\t");
		    ios << d << "\n";
		    ios << hate::Indentation();
		    ios << ")";
	    },
	    data);
	return os;
}


PlasticityRule::PlasticityRule(
    std::optional<RecordingConfig> const& recording,
    ID const& id,
    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>>
        population_shapes,
    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>>
        projection_shapes,
    grenade::common::TimeDomainOnTopology time_domain,
    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
        execution_instance_on_executor) :
    PartitionedVertex(execution_instance_on_executor),
    recording(recording),
    id(id),
    m_time_domain(time_domain)
{
	for (auto const& population_shape : population_shapes) {
		m_population_shapes.push_back(population_shape.get());
	}
	for (auto const& projection_shape : projection_shapes) {
		m_projection_shapes.push_back(projection_shape.get());
	}
}

bool PlasticityRule::valid_input_port_data(
    size_t input_port_on_vertex, grenade::common::PortData const& data) const
{
	return (input_port_on_vertex == m_population_shapes.size() + m_projection_shapes.size() &&
	        dynamic_cast<Parameterization const*>(&data) != nullptr) ||
	       (input_port_on_vertex == (m_population_shapes.size() + m_projection_shapes.size() + 1) &&
	        dynamic_cast<Dynamics const*>(&data) != nullptr);
}

bool PlasticityRule::valid(grenade::common::TimeDomainRuntimes const& time_domain_runtimes) const
{
	return dynamic_cast<ClockCycleTimeDomainRuntimes const*>(&time_domain_runtimes) != nullptr;
}

bool PlasticityRule::valid_output_port_data(
    size_t output_port_on_vertex, grenade::common::PortData const& data) const
{
	return output_port_on_vertex == 0 &&
	       dynamic_cast<PlasticityRule::Results const*>(&data) != nullptr;
}

void PlasticityRule::set_population_shapes(
    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>> value)
{
	m_population_shapes.clear();
	for (auto const& v : value) {
		m_population_shapes.push_back(v.get());
	}
}

void PlasticityRule::set_projection_shapes(
    std::vector<std::reference_wrapper<grenade::common::MultiIndexSequence const>> value)
{
	m_projection_shapes.clear();
	for (auto const& v : value) {
		m_projection_shapes.push_back(v.get());
	}
}

std::optional<grenade::common::TimeDomainOnTopology> PlasticityRule::get_time_domain() const
{
	return m_time_domain;
}

void PlasticityRule::set_time_domain(grenade::common::TimeDomainOnTopology const& value)
{
	m_time_domain = value;
}

std::vector<PlasticityRule::Port> PlasticityRule::get_input_ports() const
{
	std::vector<Port> ret;
	for (auto const& projection_shape : m_projection_shapes) {
		ret.push_back(Port(
		    SynapseObservable(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::no, *projection_shape));
	}
	for (auto const& population_shape : m_population_shapes) {
		ret.push_back(Port(
		    AnalogObservable(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::not_supported,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::no, *population_shape));
	}
	ret.push_back(grenade::common::Vertex::Port(
	    ParameterizationPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::CuboidMultiIndexSequence({1})));
	ret.push_back(grenade::common::Vertex::Port(
	    DynamicsPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::no,
	    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
	    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
	    grenade::common::CuboidMultiIndexSequence({1})));
	return ret;
}

std::vector<PlasticityRule::Port> PlasticityRule::get_output_ports() const
{
	if (recording) {
		return {grenade::common::Vertex::Port(
		    ResultsPortType(), grenade::common::Vertex::Port::SumOrSplitSupport::yes,
		    grenade::common::Vertex::Port::ExecutionInstanceTransitionConstraint::required,
		    grenade::common::Vertex::Port::RequiresOrGeneratesData::yes,
		    grenade::common::CuboidMultiIndexSequence({1}))};
	}
	return {};
}

std::ostream& PlasticityRule::print(std::ostream& os) const
{
	hate::IndentingOstream ios(os);
	ios << "PlasticityRule(\n";
	ios << hate::Indentation("\t");
	PartitionedVertex::print(ios) << "\n";
	ios << m_time_domain << "\n";
	ios << "population_shapes: " << hate::join(m_population_shapes, ", ") << "\n";
	ios << "projection_shapes: " << hate::join(m_projection_shapes, ", ") << "\n";
	ios << hate::Indentation();
	ios << ")";
	return os;
}

bool PlasticityRule::is_equal_to(Vertex const& other) const
{
	auto const& other_plasticity_rule = static_cast<PlasticityRule const&>(other);
	return recording == other_plasticity_rule.recording && id == other_plasticity_rule.id &&
	       m_population_shapes == other_plasticity_rule.m_population_shapes &&
	       m_projection_shapes == other_plasticity_rule.m_projection_shapes &&
	       m_time_domain == other_plasticity_rule.m_time_domain &&
	       PartitionedVertex::is_equal_to(other);
}

std::unique_ptr<grenade::common::Vertex> PlasticityRule::copy() const
{
	return std::make_unique<PlasticityRule>(*this);
}

std::unique_ptr<grenade::common::Vertex> PlasticityRule::move()
{
	return std::make_unique<PlasticityRule>(std::move(*this));
}

} // namespace grenade::vx::network::abstract
