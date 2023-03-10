#include "grenade/vx/network/placed_logical/routing/source_on_padi_bus_manager.h"

#include "grenade/vx/network/placed_logical/routing/detail/source_on_padi_bus_manager.h"
#include "grenade/vx/network/placed_logical/routing/detail/source_on_padi_bus_manager.tcc"
#include "halco/common/iter_all.h"
#include "hate/indent.h"
#include "hate/join.h"
#include "hate/math.h"
#include "lola/vx/v3/synapse.h"
#include <sstream>
#include <log4cxx/logger.h>

namespace grenade::vx::network::placed_logical::routing {

using namespace halco::common;
using namespace halco::hicann_dls::vx::v3;

PADIBusOnPADIBusBlock SourceOnPADIBusManager::InternalSource::toPADIBusOnPADIBusBlock() const
{
	return PADIBusOnPADIBusBlock(neuron.toNeuronColumnOnDLS()
	                                 .toNeuronEventOutputOnDLS()
	                                 .toNeuronEventOutputOnNeuronBackendBlock());
}

bool SourceOnPADIBusManager::Partition::Group::valid() const
{
	static auto logger =
	    log4cxx::Logger::getLogger("grenade.SourceOnPADIBusManager.Partition.Group");
	std::set<size_t> unique(sources.begin(), sources.end());
	if (unique.size() != sources.size()) {
		LOG4CXX_ERROR(logger, "valid(): Partitioned source group indices are not unique.");
		return false;
	}
	if (sources.size() > lola::vx::v3::SynapseMatrix::Label::size) {
		LOG4CXX_ERROR(logger, "valid(): Partitioned source group indices are too many.");
		return false;
	}
	if (!allocation_request.valid()) {
		LOG4CXX_ERROR(
		    logger,
		    "valid(): Partitioned source group synapse driver allocation request not valid.");
		return false;
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, SourceOnPADIBusManager::Partition::Group const& config)
{
	os << "Group(\n";
	std::stringstream ss;
	ss << hate::join_string(config.sources.begin(), config.sources.end(), ", ") << "\n";
	ss << config.allocation_request;
	os << hate::indent(ss.str(), "\t") << "\n";
	os << ")";
	return os;
}

bool SourceOnPADIBusManager::Partition::valid() const
{
	auto const valid_single = [](auto const& groups) {
		for (auto const& group : groups) {
			if (!group.valid()) {
				return false;
			}
		}
		return true;
	};
	return valid_single(internal) && valid_single(background) && valid_single(external);
}

std::ostream& operator<<(std::ostream& os, SourceOnPADIBusManager::Partition const& config)
{
	os << "Partition(\n";
	os << "\tinternal:\n";
	if (!config.internal.empty()) {
		os << hate::indent(
		          hate::join_string(config.internal.begin(), config.internal.end(), "\n"), "\t\t")
		   << "\n";
	}
	os << "\tbackground:\n";
	if (!config.background.empty()) {
		os << hate::indent(
		          hate::join_string(config.background.begin(), config.background.end(), "\n"),
		          "\t\t")
		   << "\n";
	}
	os << "\texternal:\n";
	if (!config.external.empty()) {
		os << hate::indent(
		          hate::join_string(config.external.begin(), config.external.end(), "\n"), "\t\t")
		   << "\n";
	}
	os << ")";
	return os;
}

SourceOnPADIBusManager::SourceOnPADIBusManager(
    DisabledInternalRoutes const& disabled_internal_routes) :
    m_logger(log4cxx::Logger::getLogger("grenade.SourceOnPADIBusManager")),
    m_disabled_internal_routes(disabled_internal_routes)
{}

std::optional<SourceOnPADIBusManager::Partition> SourceOnPADIBusManager::solve(
    std::vector<InternalSource> const& internal_sources,
    std::vector<BackgroundSource> const& background_sources,
    std::vector<ExternalSource> const& external_sources) const
{
	// sort internal sources to PADI-busses per PADI-bus block
	typed_array<
	    typed_array<std::vector<size_t>, PADIBusOnPADIBusBlock>, NeuronBackendConfigBlockOnDLS>
	    internal_sources_per_padi_bus;
	for (size_t i = 0; i < internal_sources.size(); ++i) {
		internal_sources_per_padi_bus[internal_sources.at(i)
		                                  .neuron.toNeuronColumnOnDLS()
		                                  .toNeuronEventOutputOnDLS()
		                                  .toNeuronBackendConfigBlockOnDLS()]
		                             [internal_sources.at(i).toPADIBusOnPADIBusBlock()]
		                                 .push_back(i);
	}
	// log number of sources per PADI-bus
	for (auto const backend_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
		for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
			LOG4CXX_DEBUG(
			    m_logger, "Got " << internal_sources_per_padi_bus[backend_block][padi_bus].size()
			                     << " internal sources on (" << backend_block << ", " << padi_bus
			                     << ").");
		}
	}

	// split internal sources linearly into chunks of same row address but different synapse label
	typed_array<
	    typed_array<std::vector<std::vector<size_t>>, PADIBusOnPADIBusBlock>,
	    NeuronBackendConfigBlockOnDLS>
	    split_internal_sources_per_padi_bus;
	for (auto const backend_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
		for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
			split_internal_sources_per_padi_bus[backend_block][padi_bus] =
			    detail::SourceOnPADIBusManager::split_linear(
			        internal_sources_per_padi_bus[backend_block][padi_bus]);
			// log number of source chunks per PADI-bus
			LOG4CXX_DEBUG(
			    m_logger,
			    "Got " << split_internal_sources_per_padi_bus[backend_block][padi_bus].size()
			           << " internal source chunks on (" << backend_block << ", " << padi_bus
			           << ").");
		}
	}

	// calculate required number of synapse drivers per chunk of internal sources
	typed_array<typed_array<std::vector<size_t>, PADIBusOnDLS>, NeuronBackendConfigBlockOnDLS>
	    split_internal_num_synapse_drivers;
	for (auto const backend_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
		for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
			for (auto const& split : split_internal_sources_per_padi_bus[backend_block][padi_bus]) {
				auto const num_synapse_drivers =
				    detail::SourceOnPADIBusManager::get_num_synapse_drivers(
				        internal_sources, split);
				for (auto const padi_bus_block : iter_all<PADIBusBlockOnDLS>()) {
					split_internal_num_synapse_drivers[backend_block][PADIBusOnDLS(
					                                                      padi_bus, padi_bus_block)]
					    .push_back(num_synapse_drivers[padi_bus_block]);
				}
			}
			// log accumulated number of synapse drivers required for chunks per PADI-bus
			for (auto const padi_bus_block : iter_all<PADIBusBlockOnDLS>()) {
				LOG4CXX_DEBUG(
				    m_logger, "Got "
				                  << std::accumulate(
				                         split_internal_num_synapse_drivers
				                             [backend_block][PADIBusOnDLS(padi_bus, padi_bus_block)]
				                                 .begin(),
				                         split_internal_num_synapse_drivers
				                             [backend_block][PADIBusOnDLS(padi_bus, padi_bus_block)]
				                                 .end(),
				                         0)
				                  << " required synapse drivers for internal source chunks on ("
				                  << backend_block << ", " << PADIBusOnDLS(padi_bus, padi_bus_block)
				                  << ").");
			}
		}
	}

	// sort background sources to PADI-busses
	typed_array<std::vector<size_t>, PADIBusOnDLS> background_sources_per_padi_bus;
	for (size_t i = 0; i < background_sources.size(); ++i) {
		background_sources_per_padi_bus[background_sources.at(i).padi_bus].push_back(i);
	}
	// log number of sources per PADI-bus
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		LOG4CXX_DEBUG(
		    m_logger, "Got " << background_sources_per_padi_bus[padi_bus].size()
		                     << " background sources on " << padi_bus << ".");
	}

	// split background sources linearly into chunks of same row address but different synapse label
	typed_array<std::vector<std::vector<size_t>>, PADIBusOnDLS>
	    split_background_sources_per_padi_bus;
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		split_background_sources_per_padi_bus[padi_bus] =
		    detail::SourceOnPADIBusManager::split_linear(background_sources_per_padi_bus[padi_bus]);
		// log number of source chunks per PADI-bus
		LOG4CXX_DEBUG(
		    m_logger, "Got " << split_background_sources_per_padi_bus[padi_bus].size()
		                     << " background source chunks on " << padi_bus << ".");
	}

	// calculate required number of synapse drivers per chunk of background sources
	typed_array<std::vector<size_t>, PADIBusOnDLS> split_background_num_synapse_drivers;
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		for (auto const& split : split_background_sources_per_padi_bus[padi_bus]) {
			auto const num_synapse_drivers =
			    detail::SourceOnPADIBusManager::get_num_synapse_drivers(background_sources, split);
			split_background_num_synapse_drivers[padi_bus].push_back(
			    num_synapse_drivers[padi_bus.toPADIBusBlockOnDLS()]);
		}
		// log accumulated number of synapse drivers required for chunks per PADI-bus
		LOG4CXX_DEBUG(
		    m_logger, "Got " << std::accumulate(
		                            split_background_num_synapse_drivers[padi_bus].begin(),
		                            split_background_num_synapse_drivers[padi_bus].end(), 0)
		                     << " required synapse drivers for background source chunks on "
		                     << padi_bus << ".");
	}

	// distribute external sources linearly onto available synapse drivers on the PADI-busses
	typed_array<size_t, PADIBusOnDLS> used_num_synapse_drivers;
	used_num_synapse_drivers.fill(0);
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		for (auto const backend_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
			auto const& internal = split_internal_num_synapse_drivers[backend_block][padi_bus];
			used_num_synapse_drivers[padi_bus] +=
			    std::accumulate(internal.begin(), internal.end(), 0);
		}
		auto const& background = split_background_num_synapse_drivers[padi_bus];
		used_num_synapse_drivers[padi_bus] +=
		    std::accumulate(background.begin(), background.end(), 0);
	}
	if (std::any_of(
	        used_num_synapse_drivers.begin(), used_num_synapse_drivers.end(),
	        [](auto const& num) { return num > SynapseDriverOnPADIBus::size; })) {
		// Internal and background sources together require too many synapse drivers on a single
		// PADI-bus.
		return std::nullopt;
	}
	auto const split_external_sources_per_padi_bus =
	    detail::SourceOnPADIBusManager::distribute_external_sources_linear(
	        external_sources, used_num_synapse_drivers);
	if (!split_external_sources_per_padi_bus) {
		// Not all external sources could be distributed -> return unsuccessful
		return std::nullopt;
	}
	// log number of source chunks per PADI-bus
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		LOG4CXX_DEBUG(
		    m_logger, "Got " << (*split_external_sources_per_padi_bus)[padi_bus].size()
		                     << " external source chunks on " << padi_bus << ".");
	}

	// calculate required number of synapse drivers used by external sources
	typed_array<std::vector<size_t>, PADIBusOnDLS> split_external_num_synapse_drivers;
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		for (auto const& split : (*split_external_sources_per_padi_bus)[padi_bus]) {
			auto const num_synapse_drivers =
			    detail::SourceOnPADIBusManager::get_num_synapse_drivers(external_sources, split);
			split_external_num_synapse_drivers[padi_bus].push_back(
			    num_synapse_drivers[padi_bus.toPADIBusBlockOnDLS()]);
		}
		// log accumulated number of synapse drivers required for chunks per PADI-bus
		LOG4CXX_DEBUG(
		    m_logger, "Got " << std::accumulate(
		                            split_external_num_synapse_drivers[padi_bus].begin(),
		                            split_external_num_synapse_drivers[padi_bus].end(), 0)
		                     << " required synapse drivers for external source chunks on "
		                     << padi_bus << ".");
	}

	// build partition
	Partition partition;

	// since the dependent label groups only need to be unique, iterate its value linearly
	using DependentLabelGroup = SynapseDriverOnDLSManager::AllocationRequest::DependentLabelGroup;
	DependentLabelGroup dependent_label_group(0);
	// add allocation requests for internal sources per PADI-bus
	for (auto const padi_bus : iter_all<PADIBusOnPADIBusBlock>()) {
		for (auto const backend_block : iter_all<NeuronBackendConfigBlockOnDLS>()) {
			if (split_internal_sources_per_padi_bus[backend_block][padi_bus].empty()) {
				continue;
			}
			auto allocation_requests_internal =
			    detail::SourceOnPADIBusManager::get_allocation_requests_internal(
			        split_internal_sources_per_padi_bus[backend_block][padi_bus], padi_bus,
			        backend_block, split_internal_num_synapse_drivers[backend_block]);
			for (size_t i = 0; i < allocation_requests_internal.size(); ++i) {
				// filter-out disabled internal routes
				NeuronEventOutputOnDLS neuron_event_output(
				    NeuronEventOutputOnNeuronBackendBlock(padi_bus), backend_block);
				if (m_disabled_internal_routes.contains(neuron_event_output)) {
					for (auto const& disabled_hemisphere :
					     m_disabled_internal_routes.at(neuron_event_output)) {
						PADIBusOnDLS padi_bus_on_dls(
						    padi_bus, disabled_hemisphere.toPADIBusBlockOnDLS());
						if (allocation_requests_internal.at(i).shapes.contains(padi_bus_on_dls)) {
							if (std::any_of(
							        allocation_requests_internal.at(i)
							            .shapes.at(padi_bus_on_dls)
							            .begin(),
							        allocation_requests_internal.at(i)
							            .shapes.at(padi_bus_on_dls)
							            .end(),
							        [](auto const& shape) { return shape.size != 0; })) {
								throw std::runtime_error(
								    "Requiring event transfer across disabled internal route.");
							}
							allocation_requests_internal.at(i).shapes.erase(padi_bus_on_dls);
						}
					}
				}
				if (allocation_requests_internal.size() > 1) {
					allocation_requests_internal.at(i).dependent_label_group.emplace(
					    dependent_label_group);
				}
				Partition::Group internal_partition{
				    split_internal_sources_per_padi_bus[backend_block][padi_bus].at(i),
				    allocation_requests_internal.at(i)};
				partition.internal.push_back(internal_partition);
			}
			if (allocation_requests_internal.size() > 1) {
				dependent_label_group = DependentLabelGroup(dependent_label_group + 1);
			}
		}
	}
	// add allocation requests for background sources per PADI-bus
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		if (split_background_sources_per_padi_bus[padi_bus].empty()) {
			continue;
		}
		auto allocation_requests_background =
		    detail::SourceOnPADIBusManager::get_allocation_requests_background(
		        split_background_sources_per_padi_bus[padi_bus], padi_bus,
		        split_background_num_synapse_drivers[padi_bus]);
		for (size_t i = 0; i < allocation_requests_background.size(); ++i) {
			if (allocation_requests_background.size() > 1) {
				allocation_requests_background.at(i).dependent_label_group.emplace(
				    dependent_label_group);
			}
			Partition::Group background_partition{
			    split_background_sources_per_padi_bus[padi_bus].at(i),
			    allocation_requests_background.at(i)};
			partition.background.push_back(background_partition);
		}
		if (allocation_requests_background.size() > 1) {
			dependent_label_group = DependentLabelGroup(dependent_label_group + 1);
		}
	}
	// add allocation requests for external sources per PADI-bus
	for (auto const padi_bus : iter_all<PADIBusOnDLS>()) {
		for (size_t i = 0; i < (*split_external_sources_per_padi_bus)[padi_bus].size(); ++i) {
			Partition::Group external_partition{
			    (*split_external_sources_per_padi_bus)[padi_bus].at(i),
			    detail::SourceOnPADIBusManager::get_allocation_requests_external(
			        padi_bus, split_external_num_synapse_drivers[padi_bus].at(i))};
			partition.external.push_back(external_partition);
		}
	}

	if (!partition.valid()) {
		auto const message = "Constructed source partitioning is not valid.";
		LOG4CXX_ERROR(m_logger, message);
		throw std::logic_error(message);
	}

	// log constructed source partition
	LOG4CXX_DEBUG(m_logger, "Constructed source partitioning: " << partition << ".");

	return partition;
}

} // namespace grenade::vx::network::placed_logical::routing
