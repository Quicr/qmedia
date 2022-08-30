#pragma once
#include <map>
#include <string>
#include <list>
#include <mutex>
#include <memory>

namespace metrics {
///
/// Measurement
///

enum struct MeasurementType {
    INFLUX = 0
};

struct Measurement {
    virtual std::string toString() = 0;
    virtual ~Measurement() = default;

    MeasurementType type() const {
        return measurement_type;
    }

protected:
    MeasurementType measurement_type;
};

/// Influx Measurement and Helpers
///

// handy defines
using Field =  std::pair<std::string, uint64_t>;
using Fields =  std::list<Field>;
using Tag = std::pair<std::string, uint64_t>;
using Tags = std::list<Tag>;
using TimePoint = std::pair<long long, Fields>;

class InfluxMeasurement : public  Measurement
{
public:

    static std::unique_ptr<InfluxMeasurement> createMeasurement(std::string name, Tags tags);

    InfluxMeasurement(std::string &name, Tags &tags);
    ~InfluxMeasurement()  = default;

    // Measurement - to line protocol
    std::string toString() override;

    struct TimeEntry
    {
        Tags tags;
        Fields fields;
    };
    using TimeSeriesEntry = std::pair<long long, TimeEntry>;

    // Setters for the measurement
    void set(std::chrono::system_clock::time_point now,
             std::list<Field> fields);
    void set(std::chrono::system_clock::time_point now, Field field);
    void set_time_entry(std::chrono::system_clock::time_point now,
                        TimeEntry &&entry);

    std::list<std::string> lineProtocol();

    int fieldIndex;
    std::map<std::string, int> fieldIds;
    std::map<int, std::string> fieldNames;
    std::string name;
    Tags tags;

    std::mutex series_lock;
    std::list<TimeSeriesEntry> series;
    std::list<TimePoint> time_points;

    std::string lineProtocol_nameAndTags();
    std::string lineProtocol_nameAndTags(Tags &tags);
    std::string lineProtocol_fields(Fields &fields);
};

}
