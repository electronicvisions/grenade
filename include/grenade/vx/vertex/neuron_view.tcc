namespace grenade::vx::vertex {

template <typename ColumnsT, typename RowT>
NeuronView::NeuronView(ColumnsT&& columns, RowT&& row) : m_columns(), m_row(std::forward<RowT>(row))
{
	check(columns);
	m_columns = std::forward<ColumnsT>(columns);
}

} // namespace grenade::vx::vertex
