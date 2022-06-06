#pragma once

#include <cstdint>
#if defined(__linux__) || defined(__APPLE__)
#define EXPORT
#define CALL
#elif _WIN32
#define EXPORT __declspec(dllexport)
#define CALL __stdcall
#endif

extern "C"
{
    /// <summary>
    /// C callback for Neo::callbackSourceId.
    /// </summary>
    typedef void(CALL *SourceCallback)(std::uint64_t client,
                                       std::uint64_t source,
                                       std::uint64_t start_time,
                                       int source_type);

    typedef void(CALL *ExternLogCallback)(const char *message);

    /// <summary>
    /// Represents a Neo instance.
    /// </summary>
    typedef void *NeoMediaInstance;

    EXPORT NeoMediaInstance CALL Init(const char *remote_address,
                                      std::uint16_t remote_port,
                                      std::uint16_t audio_sample_rate,
                                      std::uint16_t audio_channels,
                                      std::uint16_t audio_type,
                                      std::uint16_t video_max_width,
                                      std::uint16_t video_max_height,
                                      std::uint16_t video_max_frame_rate,
                                      std::uint32_t video_max_bitrate,
                                      std::uint16_t video_encode_pixel_format,
                                      std::uint16_t video_decode_pixel_format,
                                      std::uint64_t client_id,
                                      std::uint64_t conference_id,
                                      SourceCallback callback,
                                      std::uint16_t transportType,
                                      ExternLogCallback log,
                                      std::uint16_t media_direction,
                                      bool echo = false);

    EXPORT void CALL sendAudio(NeoMediaInstance instance,
                               const char *buffer,
                               std::uint16_t length,
                               std::uint64_t timestamp,
                               std::uint64_t sourceID);

    EXPORT int CALL getAudio(NeoMediaInstance instance,
                             std::uint64_t client_id,
                             std::uint64_t source_id,
                             std::uint64_t &timestamp,
                             unsigned char **buffer,
                             std::uint16_t max_len,
                             void **packet_to_free);

    EXPORT void CALL sendVideoFrame(NeoMediaInstance instance,
                                    const char *buffer,
                                    std::uint32_t length,
                                    std::uint32_t width,
                                    std::uint32_t height,
                                    std::uint32_t stride_y,
                                    std::uint32_t stride_uv,
                                    std::uint32_t offset_u,
                                    std::uint32_t offset_v,
                                    std::uint32_t format,
                                    std::uint64_t timestamp,
                                    std::uint64_t sourceID);

    EXPORT std::uint32_t CALL getVideoFrame(NeoMediaInstance instance,
                                            std::uint64_t client_id,
                                            std::uint64_t source_id,
                                            std::uint64_t &timestamp,
                                            std::uint32_t &width,
                                            std::uint32_t &height,
                                            std::uint32_t &format,
                                            unsigned char **buffer);

    EXPORT void CALL setLoopbackMode(NeoMediaInstance instance,
                                     std::uint8_t mode);

    EXPORT void CALL freePacket(void *packet);

    EXPORT void CALL Destroy(NeoMediaInstance *instance);

    EXPORT void CALL setMicrophoneMute(NeoMediaInstance instance, bool muted);

    // --- QUICR Specific APIs ---
    // Pub/Sub Api [ used only for quicr transport ]
    // Adds a publisher source with the given url and media type
    // Note: Media needs to be sent via sendXXX(..) functions
    EXPORT void CALL publish(NeoMediaInstance instance,
                             std::uint64_t source_id,
                             std::uint16_t media_type,
                             const char *url,
                             std::uint16_t url_length);

    // Adds a subscriber for a given url and media type
    EXPORT void CALL subscribe(NeoMediaInstance instance,
                               std::uint64_t source_id,
                               std::uint16_t media_type,
                               const char *url,
                               std::uint16_t url_length);
    // Explicit call to kick off the transport.
    // Needed for transports
    EXPORT void CALL start_transport(NeoMediaInstance instance,
                                     std::uint16_t transport_type);
}
