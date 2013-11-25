#ifndef TIMESTAMP_HPP_SFETMZJI
#define TIMESTAMP_HPP_SFETMZJI

#include <cstdint>
#include <boost/operators.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/units/absolute.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/systems/si/time.hpp>
#include <boost/units/make_scaled_unit.hpp>

namespace vanetza
{
namespace geonet
{

class Timestamp : public boost::totally_ordered<Timestamp>
{
public:
    typedef uint32_t value_type;
    typedef boost::units::make_scaled_unit<
                boost::units::si::time,
                boost::units::scale<10, boost::units::static_rational<-3>>
            >::type unit_type;
    typedef boost::units::absolute<unit_type> absolute_unit_type;
    typedef boost::units::quantity<absolute_unit_type, value_type> time_type;
    typedef boost::units::quantity<unit_type, value_type> duration_type;

    static const unit_type millisecond;
    static const boost::posix_time::ptime start_time;

    Timestamp() : m_timestamp(0) {}
    Timestamp(time_type t) : m_timestamp(t) {}
    Timestamp(const boost::posix_time::ptime&);
    value_type raw() const { return m_timestamp.value(); }
    time_type quantity() const { return m_timestamp; }

private:
    time_type m_timestamp; // since 01-01-2004 00:00:00.0
};

// timestamp comparison according to ETSI GeoNet
bool is_greater(Timestamp lhs, Timestamp rhs);

// operators required for boost::totally_ordered
bool operator<(Timestamp lhs, Timestamp rhs);
bool operator==(Timestamp lhs, Timestamp rhs);

} // namespace geonet
} // namespace vanetza

#endif /* TIMESTAMP_HPP_SFETMZJI */

