#pragma once
#include "dapr/empty_property.h"
#include "dapr/property_holder.h"
#include "grenade/common/multi_index_sequence.h"
#include "grenade/common/partitioned_vertex.h"
#include "grenade/common/port_data/batched.h"
#include "grenade/vx/common/timed_data.h"
#include "grenade/vx/signal_flow/connection_type.h"
#include "grenade/vx/signal_flow/event.h"
#include "grenade/vx/signal_flow/types.h"
#include "hate/visibility.h"
#include <functional>
#include <memory>
#include <ostream>
#include <stddef.h>
#include <variant>
#include <vector>

namespace cereal {
struct access;
} // namespace cereal

namespace grenade::vx::signal_flow::vertex {

/**
 * Formatted data input from memory.
 */
struct SYMBOL_VISIBLE Transformation : public grenade::common::PartitionedVertex
{
	struct Dynamics : public grenade::common::BatchedPortData
	{
		typedef std::variant<
		    std::vector<common::TimedDataSequence<std::vector<UInt32>>>,
		    std::vector<common::TimedDataSequence<std::vector<UInt5>>>,
		    std::vector<common::TimedDataSequence<std::vector<Int8>>>,
		    std::vector<TimedSpikeToChipSequence>,
		    std::vector<TimedSpikeFromChipSequence>,
		    std::vector<TimedMADCSampleFromChipSequence>>
		    Value;
		Value value;

		Dynamics() = default;
		Dynamics(Value value);

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	/**
	 * Dynamics port type.
	 */
	struct SYMBOL_VISIBLE GENPYBIND(inline_base("*EmptyProperty*")) DynamicsPortType
	    : public dapr::EmptyProperty<DynamicsPortType, grenade::common::VertexPortType>
	{};

	struct Results : public grenade::common::BatchedPortData
	{
		typedef Dynamics::Value Value;
		Value value;

		Results(Value value);

		virtual size_t batch_size() const override;

		virtual std::unique_ptr<PortData> copy() const override;
		virtual std::unique_ptr<PortData> move() override;

		/**
		 * Get section of values. Sequence needs to be in [0, size).
		 */
		std::unique_ptr<Results> get_section(
		    grenade::common::MultiIndexSequence const& sequence) const;

	protected:
		virtual std::ostream& print(std::ostream& os) const override;
		virtual bool is_equal_to(PortData const& other) const override;
	};

	struct SYMBOL_VISIBLE Function : public dapr::Property<Function>
	{
		struct Port
		{
			Port(
			    grenade::common::Vertex::Port::Type const& type,
			    grenade::common::MultiIndexSequence const& channels);

			grenade::common::Vertex::Port::Type const& get_type() const;
			void set_type(grenade::common::Vertex::Port::Type const& value);

			grenade::common::MultiIndexSequence const& get_channels() const;
			void set_channels(grenade::common::MultiIndexSequence const& value);

			bool operator==(Port const& other) const = default;
			bool operator!=(Port const& other) const = default;

			friend std::ostream& operator<<(std::ostream& os, Port const& value) SYMBOL_VISIBLE;

		private:
			dapr::PropertyHolder<grenade::common::Vertex::Port::Type> m_type;
			dapr::PropertyHolder<grenade::common::MultiIndexSequence> m_channels;
		};

		virtual ~Function();

		virtual std::vector<Function::Port> get_input_ports() const = 0;
		virtual std::vector<Function::Port> get_output_ports() const = 0;

		/**
		 * Apply function to input data.
		 * @param input_data Input data
		 * @return Transformed output data
		 */
		virtual std::vector<Results> apply(
		    std::vector<std::reference_wrapper<grenade::common::PortData const>> const& input_data)
		    const = 0;

		virtual bool valid_input_port_data(
		    size_t input_port_on_function, grenade::common::PortData const& data) const;

	private:
		friend struct cereal::access;
		template <typename Archive>
		void serialize(Archive& ar, std::uint32_t);
	};

	/**
	 * Construct Transformation.
	 * @param function Function to apply during execution
	 * @param execution_instance_on_executor Execution instance to use
	 */
	Transformation(
	    Function const& function,
	    std::optional<grenade::common::ExecutionInstanceOnExecutor> const&
	        execution_instance_on_executor = std::nullopt);

	Function const& get_function() const;
	void set_function(Function const& value);

	virtual bool valid_output_port_data(
	    size_t output_port_on_vertex, grenade::common::PortData const& results) const override;

	virtual bool valid_input_port_data(
	    size_t input_port_on_vertex, grenade::common::PortData const& data) const override;

	virtual std::vector<Port> get_input_ports() const override;
	virtual std::vector<Port> get_output_ports() const override;

	virtual std::unique_ptr<Vertex> copy() const override;
	virtual std::unique_ptr<Vertex> move() override;

protected:
	virtual bool is_equal_to(Vertex const& other) const override;
	virtual std::ostream& print(std::ostream& os) const override;

private:
	dapr::PropertyHolder<Function> m_function;

	friend struct cereal::access;
	template <typename Archive>
	void serialize(Archive& ar, std::uint32_t);
};

} // grenade::vx::signal_flow::vertex
