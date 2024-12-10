#pragma once

#include <functional>

namespace marengo {
namespace amaze {

// Add event names to this as required. It's just more type safe than using
// a string to name events being added.
enum class ScheduleEventName {
    EndLoops,
    Exploding,
    BreakableExploding
};

struct ScheduleItem {
    ScheduleEventName name;
    int numberOfFrames { 0 };
    std::function<void()> doWhile;
    std::function<void()> doAfter;
};

class Scheduler {
public:
    void doWhile(ScheduleEventName name, int frames, std::function<void()> fn);
    void doAfter(ScheduleEventName name, int frames, std::function<void()> fn);
    void addScheduleItem(ScheduleItem&& si);
    bool isScheduleItemActive(ScheduleEventName name);
    void processSchedule();

private:
    std::vector<ScheduleItem> m_scheduleItems;
};

} // namespace amaze
} // namespace marengo