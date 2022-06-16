#include "playout_sync.hh"

using namespace neo_media;

// make a note about most recent audio popped by the cloent
void Sync::audio_popped(uint64_t source_time,
                        uint64_t seq,
                        std::chrono::steady_clock::time_point now)
{
    source_audio_time_popped = source_time;
    audio_seq_popped = seq;
    local_audio_time_popped = now;
}

void Sync::video_popped(uint64_t source_time,
                        uint64_t seq,
                        std::chrono::steady_clock::time_point now)
{
    source_video_time_popped = source_time;
    video_seq_popped = seq;
    local_video_time_popped = now;
}

Sync::sync_action Sync::getVideoAction(unsigned int audio_pop_delay,
                                       unsigned int video_pop_delay,
                                       const MetaQueue &mq,
                                       unsigned int &num_pop,
                                       std::chrono::steady_clock::time_point now)
{
    sync_action action = sync_action::hold;
    num_pop = 0;

    for (const auto &frame : mq.Q)
    {
        logger->info << "[sync:videoAction num_pop:" << num_pop
                     << ", video_seq_popped:" << video_seq_popped << "]" << std::flush;

        if(frame->packet != nullptr) {
            logger->debug << "\t " << *frame->packet << std::flush;
        } else {
            logger->info << " null packet pointer" << std::flush;
            continue;
        }

        // first frame - pop_discard to IDR - or pop if IDR is next
        if (video_seq_popped == UINT64_MAX)
        {
            if (frame->packet->videoFrameType != Packet::VideoFrameType::Idr)
            {
                logger->info << "[sync:videoAction: First frame "
                             << frame->packet->encodedSequenceNum
                             << " is not idr, discard]" << std::flush;
                action = pop_discard;
                ++num_pop;
            }
            else
            {
                if (action != pop_discard)
                {
                    action = pop;
                    ++num_pop;
                }
                break;
            }
        }

        // video in order
        // TODO:  when temporal layers are introduced - use frame references
        // instead of sequence numbers
        else if (frame->packet->encodedSequenceNum ==
                 (video_seq_popped + num_pop + 1))
        {
            logger->info << "[sync:videoAction: processing frames in order ]" << std::flush;
            // video only action
            if (source_audio_time_popped == 0)
            {
                action = sync_action::pop_video_only;
                ++num_pop;
            }
            // if older than audio - pop this frame
            else if (frame->packet && frame->packet->sourceRecordTime < source_audio_time_popped)
            {
                // pop (another) older frame
                action = sync_action::pop;
                ++num_pop;
                logger->info << "[sync:videoAction popping older frame: num_pop:"
                             << num_pop << "]" << std::flush;
            }
            // audio stopped popping or stopped receiving requires independent
            // video action - 400ms is trigger
            else if (std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - local_audio_time_popped)
                         .count() > 400)
            {
                action = sync_action::pop_video_only;
                ++num_pop;
                logger->info << "[sync:videoAction audio pop stopped > 400ms" << std::flush;
                break;        // only single pop for video only
            }
            else {
                logger->info << "[sync:videoAction, returning default action:" << action << std::flush;
                return action;  // hard exit - we have frames in order.
                                // return either previous pop or default
                                 // hold
            }


        }
        // video out of order - discard until IDR is found - or return IDR
        else
        {
            logger->info << "[sync:videoAction processing out of order frame: action"
                << (int) action << "]" << std::flush;
            if (action == pop)   {
                logger->info << "sync:videoAction:OOO: action==pop" << std::flush;
                break;
            }     // if we find out-of-order deeper in the queue - wait

            // TODO:  take into consideration type of frame to support
            // discardable (acceptable) loss
            if (frame->packet->videoFrameType != Packet::VideoFrameType::Idr)
            {
                logger->info << "[sync:videoAction discarding, not IDR"
                             << (int) frame->packet->videoFrameType << std::flush;
                action = pop_discard;
                ++num_pop;
            }
            else
            {
                if (action != pop_discard)
                {
                    action = pop;
                    ++num_pop;
                    logger->info << "[sync:videoAction setting action to pop, num_pop"
                                 << num_pop << "]" << std::flush;
                }
                break;
            }
        }
    }

    logger->info << "[sync:videoAction final action:" << action << "]" << std::flush;
    return action;
}
