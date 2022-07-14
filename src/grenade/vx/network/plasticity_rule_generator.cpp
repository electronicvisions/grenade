#include "grenade/vx/network/plasticity_rule_generator.h"

#include "grenade/vx/io_data_map.h"
#include "grenade/vx/network/network_graph.h"
#include "halco/common/iter_all.h"
#include "halco/common/typed_array.h"
#include "halco/hicann-dls/vx/v3/ppu.h"
#include <sstream>

namespace grenade::vx::network {

OnlyRecordingPlasticityRuleGenerator::OnlyRecordingPlasticityRuleGenerator(
    std::set<Observable> const& observables) :
    m_observables(observables)
{}

PlasticityRule OnlyRecordingPlasticityRuleGenerator::generate() const
{
	std::stringstream kernel;

	kernel << "#include \"grenade/vx/ppu/synapse_array_view_handle.h\"\n";
	kernel << "#include \"libnux/vx/location.h\"\n";
	kernel << "#include \"libnux/vx/correlation.h\"\n";
	kernel << "#include \"libnux/vx/vector_convert.h\"\n";
	kernel << "#include \"libnux/vx/time.h\"\n";
	kernel << "#include \"libnux/vx/helper.h\"\n";
	kernel << "#include \"hate/tuple.h\"\n";

	kernel << "using namespace grenade::vx::ppu;\n";
	kernel << "using namespace libnux::vx;\n";

	kernel << "extern uint64_t time_origin;\n";
	kernel << "extern volatile PPUOnDLS ppu;\n";

	kernel << "template <size_t N>\n";
	kernel << "void PLASTICITY_RULE_KERNEL(std::array<SynapseArrayViewHandle, N>& synapses, "
	          "std::array<PPUOnDLS, N> synrams, Recording& recording)\n";
	kernel << "{\n";

	kernel << "  recording.time = now() - time_origin;\n";

	if (m_observables.contains(Observable::weights)) {
		kernel << "        {\n";
		kernel << "          hate::for_each([&](auto& "
		          "weights_per_synapse_view, auto& synapse, auto const& synram) {\n";
		kernel << "            if (synram == ppu) {\n";
		kernel << "              size_t row = 0;\n";
		kernel << "              for (size_t i = 0; i < synapse.rows.size; ++i) {\n";
		kernel << "                if (synapse.rows.test(i)) {\n";
		kernel << "                  auto const tmp = "
		       << "static_cast<VectorRowFracSat8>(synapse.get_weights(i));\n";
		kernel << "                  size_t column = 0;\n";
		kernel << "                  for (size_t j = 0; j < synapse.columns.size; ++j) {\n";
		kernel << "                    if (synapse.columns.test(j)) {\n";
		kernel << "                      weights_per_synapse_view[row][column] = tmp[j];\n";
		kernel << "                      column++;\n";
		kernel << "                    }\n";
		kernel << "                  }\n";
		kernel << "                  do_not_optimize_away(weights_per_synapse_view[row]);\n";
		kernel << "                  do_not_optimize_away(weights_per_synapse_view);\n";
		kernel << "                  row++;\n";
		kernel << "                }\n";
		kernel << "              }\n";
		kernel << "            }\n";
		kernel << "          }, recording.weights, synapses, synrams);\n";
		kernel << "        }\n";
	}
	if (m_observables.contains(Observable::correlation_causal) &&
	    !m_observables.contains(Observable::correlation_acausal)) {
		kernel << "        {\n";
		kernel << "          hate::for_each("
		          "[&](auto& correlation_causal_per_synapse_view, auto const& synapse, "
		          "auto const& synram) {\n";
		kernel << "            if (synram == ppu) {\n";
		kernel << "              size_t row = 0;\n";
		kernel << "              for (size_t i = 0; i < synapse.rows.size; ++i) {\n";
		kernel << "                if (synapse.rows.test(i)) {\n";
		kernel << "                  vector_row_t causal;\n";
		kernel << "                  get_causal_correlation(&(causal.even.data), "
		          "                    &(causal.odd.data), i);\n";
		kernel << "                  do_not_optimize_away(causal);\n";
		kernel << "                  reset_correlation(i);\n";
		kernel << "                  VectorRowFracSat8 const tmp = "
		          "causal.convert_contiguous();\n";
		kernel << "                  size_t column = 0;\n";
		kernel << "                  for (size_t j = 0; j < synapse.columns.size; ++j) {\n";
		kernel << "                    if (synapse.columns.test(j)) {\n";
		kernel << "                      correlation_causal_per_synapse_view[row][column] "
		          "= tmp[j];\n";
		kernel << "                      column++;\n";
		kernel << "                    }\n";
		kernel << "                  }\n";
		kernel << "                  row++;\n";
		kernel << "                }\n";
		kernel << "              }\n";
		kernel << "            }\n";
		kernel << "          }, recording.correlation_causal, synapses, synrams);\n";
		kernel << "        }\n";
	}
	if (!m_observables.contains(Observable::correlation_causal) &&
	    m_observables.contains(Observable::correlation_acausal)) {
		kernel << "        {\n";
		kernel << "          hate::for_each("
		          "[&](auto& correlation_acausal_per_synapse_view, auto const& synapse, "
		          "auto const& synram) {\n";
		kernel << "            if (synram == ppu) {\n";
		kernel << "              size_t row = 0;\n";
		kernel << "              for (size_t i = 0; i < synapse.rows.size; ++i) {\n";
		kernel << "                if (synapse.rows.test(i)) {\n";
		kernel << "                  vector_row_t acausal;\n";
		kernel << "                  get_acausal_correlation(&(acausal.even.data), "
		          "                    &(acausal.odd.data), i);\n";
		kernel << "                  do_not_optimize_away(acausal);\n";
		kernel << "                  reset_correlation(i);\n";
		kernel << "                  VectorRowFracSat8 const tmp = "
		          "acausal.convert_contiguous();\n";
		kernel << "                  size_t column = 0;\n";
		kernel << "                  for (size_t j = 0; j < synapse.columns.size; ++j) {\n";
		kernel << "                    if (synapse.columns.test(j)) {\n";
		kernel << "                      correlation_acausal_per_synapse_view[row][column] "
		          "= tmp[j];\n";
		kernel << "                      column++;\n";
		kernel << "                    }\n";
		kernel << "                  }\n";
		kernel << "                  row++;\n";
		kernel << "                }\n";
		kernel << "              }\n";
		kernel << "            }\n";
		kernel << "          }, recording.correlation_acausal, synapses, synrams);\n";
		kernel << "        }\n";
	}
	if (m_observables.contains(Observable::correlation_causal) &&
	    m_observables.contains(Observable::correlation_acausal)) {
		kernel << "        {\n";
		kernel << "          hate::for_each("
		          "[&](auto& correlation_causal_per_synapse_view, auto& "
		          "correlation_acausal_per_synapse_view, auto const& synapse, "
		          "auto const& synram) {\n";
		kernel << "            if (synram == ppu) {\n";
		kernel << "              size_t row = 0;\n";
		kernel << "              for (size_t i = 0; i < synapse.rows.size; ++i) {\n";
		kernel << "                if (synapse.rows.test(i)) {\n";
		kernel << "                  vector_row_t causal;\n";
		kernel << "                  vector_row_t acausal;\n";
		kernel << "                  get_correlation(&(causal.even.data), "
		          "                    &(causal.odd.data), &(acausal.even.data), "
		          "&(acausal.odd.data), i);\n";
		kernel << "                  do_not_optimize_away(causal);\n";
		kernel << "                  do_not_optimize_away(acausal);\n";
		kernel << "                  reset_correlation(i);\n";
		kernel << "                  VectorRowFracSat8 const tmp_causal = "
		          "causal.convert_contiguous();\n";
		kernel << "                  VectorRowFracSat8 const tmp_acausal = "
		          "acausal.convert_contiguous();\n";
		kernel << "                  size_t column = 0;\n";
		kernel << "                  for (size_t j = 0; j < synapse.columns.size; ++j) {\n";
		kernel << "                    if (synapse.columns.test(j)) {\n";
		kernel << "                      correlation_causal_per_synapse_view[row][column] "
		          "= tmp_causal[j];\n";
		kernel << "                      correlation_acausal_per_synapse_view[row][column] "
		          "= tmp_acausal[j];\n";
		kernel << "                      column++;\n";
		kernel << "                    }\n";
		kernel << "                  }\n";
		kernel << "                  row++;\n";
		kernel << "                }\n";
		kernel << "              }\n";
		kernel << "            }\n";
		kernel << "          }, recording.correlation_causal, recording.correlation_acausal, "
		          "synapses, synrams);\n";
		kernel << "        }\n";
	}

	kernel << "}\n";

	PlasticityRule rule;
	PlasticityRule::TimedRecording recording;
	for (auto const& observable : m_observables) {
		switch (observable) {
			case Observable::weights: {
				recording.observables["weights"] =
				    PlasticityRule::TimedRecording::ObservablePerSynapse{
				        PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int8,
				        PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
				            packed_active_columns};
				break;
			}
			case Observable::correlation_causal: {
				recording.observables["correlation_causal"] =
				    PlasticityRule::TimedRecording::ObservablePerSynapse{
				        PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int8,
				        PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
				            packed_active_columns};
				break;
			}
			case Observable::correlation_acausal: {
				recording.observables["correlation_acausal"] =
				    PlasticityRule::TimedRecording::ObservablePerSynapse{
				        PlasticityRule::TimedRecording::ObservablePerSynapse::Type::int8,
				        PlasticityRule::TimedRecording::ObservablePerSynapse::LayoutPerRow::
				            packed_active_columns};
				break;
			}
			default: {
				throw std::logic_error("Observable not implemented.");
			}
		}
	}
	rule.recording = recording;
	rule.kernel = kernel.str();
	return rule;
}

} // namespace grenade::vx::network
