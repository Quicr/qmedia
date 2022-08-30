#pragma once
#include <map>

///
/// Various measurements being reported within the library
///

namespace metrics {

enum struct MeasurementType
{
    PacketRate_Tx,
    PacketRate_Rx,
    FrameRate_Tx,
    FrameRate_Rx,
    QDepth_Tx,
    QDepth_Rx,
};

const auto measurement_names = std::map<MeasurementType, std::string>{
    {MeasurementType::PacketRate_Tx, "TxPacketCount"},
    {MeasurementType::PacketRate_Rx, "RxPacketCount"},
    {MeasurementType::FrameRate_Tx, "TxFrameCount"},
    {MeasurementType::FrameRate_Rx, "RxFrameCount"},
    {MeasurementType::QDepth_Tx, "TxQueueDepth"},
    {MeasurementType::QDepth_Rx, "RxQueueDepth"},
};
}