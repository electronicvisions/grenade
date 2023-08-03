namespace grenade::vx::signal_flow::vertex {

template <typename SynramT, typename RowsT, typename ColumnsT, typename WeightsT, typename LabelsT>
SynapseArrayView::SynapseArrayView(
    SynramT&& synram,
    RowsT&& rows,
    ColumnsT&& columns,
    WeightsT&& weights,
    LabelsT&& labels,
    ChipCoordinate const& chip_coordinate) :
    EntityOnChip(chip_coordinate),
    m_synram(std::forward<SynramT>(synram)),
    m_rows(),
    m_columns(),
    m_weights(),
    m_labels()
{
	check(rows, columns, weights, labels);
	m_rows = std::forward<RowsT>(rows);
	m_columns = std::forward<ColumnsT>(columns);
	m_weights = std::forward<WeightsT>(weights);
	m_labels = std::forward<LabelsT>(labels);
}

} // namespace grenade::vx::signal_flow::vertex
