namespace grenade::vx::signal_flow::vertex {

template <typename ColumnsT, typename ConfigsT, typename RowT>
NeuronView::NeuronView(
    ColumnsT&& columns, ConfigsT&& configs, RowT&& row, ChipCoordinate const& chip_coordinate) :
    EntityOnChip(chip_coordinate), m_columns(), m_row(std::forward<RowT>(row))
{
	check(columns, configs);
	m_columns = std::forward<ColumnsT>(columns);
	m_configs = std::forward<ConfigsT>(configs);
}

} // namespace grenade::vx::signal_flow::vertex
