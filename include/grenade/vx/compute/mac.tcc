namespace grenade::vx::compute {

template <typename WeightsT>
MAC::MAC(
    WeightsT&& weights,
    size_t num_sends,
    common::Time wait_between_events,
    bool enable_loopback,
    halco::hicann_dls::vx::v3::AtomicNeuronOnDLS const& madc_recording_neuron,
    std::string madc_recording_path) :
    m_enable_loopback(enable_loopback),
    m_graph(false),
    m_input_vertex(),
    m_output_vertex(),
    m_weights(std::forward<Weights>(weights)),
    m_num_sends(num_sends),
    m_wait_between_events(wait_between_events),
    m_madc_recording_neuron(madc_recording_neuron),
    m_madc_recording_path(madc_recording_path)
{
	build_graph();
}

} // namespace grenade::vx::compute
