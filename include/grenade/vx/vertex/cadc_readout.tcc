namespace grenade::vx::vertex {

template <typename ColumnsT, typename SynramT>
CADCMembraneReadoutView::CADCMembraneReadoutView(ColumnsT&& columns, SynramT&& synram) :
    m_columns(), m_synram(std::forward<SynramT>(synram))
{
	check(columns);
	m_columns = std::forward<ColumnsT>(columns);
}

} // namespace grenade::vx::vertex
