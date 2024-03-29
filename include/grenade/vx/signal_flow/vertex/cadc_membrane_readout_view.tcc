namespace grenade::vx::signal_flow::vertex {

template <typename ColumnsT, typename SynramT, typename SourcesT>
CADCMembraneReadoutView::CADCMembraneReadoutView(
    ColumnsT&& columns,
    SynramT&& synram,
    Mode const& mode,
    SourcesT&& sources,
    ChipCoordinate const& chip_coordinate) :
    EntityOnChip(chip_coordinate),
    m_columns(),
    m_synram(std::forward<SynramT>(synram)),
    m_mode(mode),
    m_sources()
{
	check(columns, sources);
	m_columns = std::forward<ColumnsT>(columns);
	m_sources = std::forward<SourcesT>(sources);
}

} // namespace grenade::vx::signal_flow::vertex
