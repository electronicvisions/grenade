namespace grenade::vx::vertex {

template <typename ColumnsT, typename EnableResetsT, typename RowT>
NeuronView::NeuronView(ColumnsT&& columns, EnableResetsT&& enable_resets, RowT&& row) :
    m_columns(), m_row(std::forward<RowT>(row))
{
	check(columns, enable_resets);
	m_columns = std::forward<ColumnsT>(columns);
	m_enable_resets = std::forward<EnableResetsT>(enable_resets);
}

} // namespace grenade::vx::vertex
