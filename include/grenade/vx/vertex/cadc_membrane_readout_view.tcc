namespace grenade::vx::vertex {

template <typename ColumnsT, typename SynramT>
CADCMembraneReadoutView::CADCMembraneReadoutView(
    ColumnsT&& columns, SynramT&& synram, Mode const& mode) :
    m_columns(), m_synram(std::forward<SynramT>(synram)), m_mode(mode)
{
	check(columns);
	m_columns = std::forward<ColumnsT>(columns);
}

} // namespace grenade::vx::vertex
