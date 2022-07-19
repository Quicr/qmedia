#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <cmath>

#include "neo_media_client.hh"
#include <qmedia/media_client.hh>

using namespace std::chrono_literals;

std::uint64_t source_id = 0;              // wait to receive something non-zero
std::uint64_t send_source_id = 37;        // random choice in send direction

void source_callback(std::uint64_t cid,
                     std::uint64_t sid,
                     std::uint64_t ts,
                     int source_type)
{
    source_id = sid;
    std::cout << std::endl
              << "Got New Source:" << sid << " Client:" << cid << " Time:" << ts
              << " Type:" << source_type << std::endl;
}

int main(int /*argc*/, char **/*argv*/)
{
    std::uint64_t client_id = 5;            // made it up
    std::uint64_t conference_id = 1;        // made it up

    std::uint32_t enc_format = 0;        // encode format: 0=NV12
    std::uint32_t dec_format = 1;        // decode format: 1=I420

    std::size_t image_width = 1280;
    std::size_t image_height = 720;
    std::size_t image_y_size = image_width * image_height;
    std::size_t image_uv_size = image_y_size >> 2;        // YUV420
    std::size_t image_size = image_y_size + image_uv_size * 2;
    char *image = static_cast<char *>(malloc(image_size));
    assert(image);

    // Fill image with color gradients.
    // 16M YUV combos mapped to 1M pixels.
    // 8 bits of Y in MSBs + 6 bits of U/V in LSBs = 20 bits total = 1M.

    for (size_t i = 0; i < image_y_size; i++)
    {
        image[i] = (i >> 12 & 0xff) + 16;        // Y in bits 12-19
    }
    for (size_t i = 0; i < image_uv_size; i++)
    {
        image[image_y_size + i * 2 + 0] = (i >> 6 & 0x3f) << 2;        // U in
                                                                       // bits
                                                                       // 6-11
        image[image_y_size + i * 2 + 1] = (i >> 0 & 0x3f) << 2;        // V in
                                                                       // bits
                                                                       // 0-5
    }

    auto logcb = [](const char *message) { std::cout << message << std::endl; };

    void *client;
    MediaClient_Create(logcb, source_callback, "127.0.0.1", 7777, &client);
    auto stream_id = MediaClient_AddVideoStream(client,
                                                0x1000,
                                                0x2000,
                                                0x3000,
                                                0,
                                                enc_format,
                                                image_width,
                                                image_height,
                                                30,
                                                200000);

    // Send frames.
    std::thread sendThread(
        [client,stream_id,  image, image_size, image_width, image_height, enc_format]()
        {
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(33));
                std::uint64_t timestamp =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
                std::cerr << " S ";        // << sourceRecordTime << "ns " <<
                                           // image_size << "bytes" <<
                                           // std::endl;

                MediaClient_sendVideoFrame(client,
                                           stream_id,
                                           image,
                                           image_size,
                                           image_width,
                                           image_height,
                                           image_width,
                                           image_width,
                                           image_width * image_height,
                                           0,
                                           enc_format,
                                           timestamp);

                image[0] = image[0] + 1;        // Embed a frame counter to
                                                // check on receive
            }
        });


    // Main thread.
    while (true)
    {
    }
}