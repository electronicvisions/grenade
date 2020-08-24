namespace grenade::vx {

template <typename WeightsT, typename RowModesT>
ComputeSingleMAC::ComputeSingleMAC(
    WeightsT&& weights,
    RowModesT&& row_modes,
    size_t num_sends,
    haldls::vx::Timer::Value wait_between_events,
    bool enable_loopback) :
    m_enable_loopback(enable_loopback),
    m_graph(false),
    m_synram_handles(),
    m_output_vertices(),
    m_weights(std::forward<Weights>(weights)),
    m_row_modes(std::forward<RowModes>(row_modes)),
    m_num_sends(num_sends),
    m_wait_between_events(wait_between_events)
{
	build_graph();
}

} // namespace grenade::vx
