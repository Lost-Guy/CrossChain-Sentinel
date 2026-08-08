#include "sentinel/core/event_scheduler.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::core {

uint64_t EventScheduler::schedule_event(uint64_t delay_ms, std::string description, EventCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t target_time = m_current_time_ms + delay_ms;
    uint64_t id = m_next_event_id++;

    m_event_queue.push(Event{
        .timestamp_ms = target_time,
        .event_id = id,
        .description = description,
        .callback = std::move(callback)
    });

    LOG_TRACE("Scheduled Event [{}] '{}' at T={}ms", id, description, target_time);
    return id;
}

size_t EventScheduler::step_until(uint64_t target_timestamp_ms) {
    size_t executed_count = 0;
    while (true) {
        Event top_event;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_event_queue.empty()) {
                m_current_time_ms = target_timestamp_ms;
                break;
            }

            top_event = m_event_queue.top();
            if (top_event.timestamp_ms > target_timestamp_ms) {
                m_current_time_ms = target_timestamp_ms;
                break;
            }

            m_event_queue.pop();
            m_current_time_ms = top_event.timestamp_ms;
        }

        if (top_event.callback) {
            top_event.callback();
        }
        executed_count++;
    }
    return executed_count;
}

bool EventScheduler::step_next() {
    Event top_event;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_event_queue.empty()) {
            return false;
        }
        top_event = m_event_queue.top();
        m_event_queue.pop();
        m_current_time_ms = top_event.timestamp_ms;
    }

    if (top_event.callback) {
        top_event.callback();
    }
    return true;
}

void EventScheduler::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_current_time_ms = 0;
    m_next_event_id = 1;
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> empty_queue;
    std::swap(m_event_queue, empty_queue);
}

size_t EventScheduler::pending_event_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_event_queue.size();
}

} // namespace sentinel::core
