#ifndef SENTINEL_CORE_EVENT_SCHEDULER_HPP
#define SENTINEL_CORE_EVENT_SCHEDULER_HPP

#include <cstdint>
#include <functional>
#include <queue>
#include <vector>
#include <mutex>
#include <string>

namespace sentinel::core {

using EventCallback = std::function<void()>;

struct Event {
    uint64_t timestamp_ms;
    uint64_t event_id;
    std::string description;
    EventCallback callback;

    // Min-heap ordering: lowest timestamp first, event_id tie-breaker
    bool operator>(const Event& other) const {
        if (timestamp_ms != other.timestamp_ms) {
            return timestamp_ms > other.timestamp_ms;
        }
        return event_id > other.event_id;
    }
};

class EventScheduler {
public:
    EventScheduler() = default;

    // Schedule an event at absolute virtual time (current_time_ms + delay_ms)
    uint64_t schedule_event(uint64_t delay_ms, std::string description, EventCallback callback);

    // Process all events up to target_timestamp_ms in strict chronological order
    size_t step_until(uint64_t target_timestamp_ms);

    // Step a single next event from priority queue
    bool step_next();

    // Reset clock and clear pending events
    void reset();

    [[nodiscard]] uint64_t current_time_ms() const { return m_current_time_ms; }
    [[nodiscard]] size_t pending_event_count() const;

private:
    uint64_t m_current_time_ms{0};
    uint64_t m_next_event_id{1};
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> m_event_queue;
    mutable std::mutex m_mutex;
};

} // namespace sentinel::core

#endif // SENTINEL_CORE_EVENT_SCHEDULER_HPP
