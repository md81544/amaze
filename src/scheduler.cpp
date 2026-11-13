#include "scheduler.h"

#include <algorithm>

namespace marengo {
namespace amaze {

void Scheduler::doWhile(ScheduleEventName name, int frames, std::function<void()> fn)
{
    if (isScheduleItemActive(name)) {
        return;
    }
    ScheduleItem si;
    si.name = name;
    si.numberOfFrames = frames;
    si.doWhile = fn;
    addScheduleItem(std::move(si));
}

void Scheduler::doAfter(ScheduleEventName name, int frames, std::function<void()> fn)
{
    if (isScheduleItemActive(name)) {
        return;
    }
    ScheduleItem si;
    si.name = name;
    si.numberOfFrames = frames;
    si.doAfter = fn;
    addScheduleItem(std::move(si));
}

void Scheduler::addScheduleItem(ScheduleItem&& si)
{
    m_scheduleItems.push_back(std::move(si));
}

bool Scheduler::isScheduleItemActive(ScheduleEventName name)
{
    for (auto& si : m_scheduleItems) {
        if (si.name == name) {
            return true;
        }
    }
    return false;
}

void Scheduler::processSchedule()
{
    // Callbacks may add new schedule items, which can reallocate the vector,
    // so iterate by index over the items present at the start, and copy each
    // callback before invoking it.
    const size_t count = m_scheduleItems.size();
    for (size_t n = 0; n < count; ++n) {
        std::function<void()> fn;
        if (--m_scheduleItems[n].numberOfFrames <= 0) {
            fn = m_scheduleItems[n].doAfter;
        } else {
            fn = m_scheduleItems[n].doWhile;
        }
        if (fn) {
            fn();
        }
    }
    // Now remove any ended items:
    m_scheduleItems.erase(
        std::remove_if(
            m_scheduleItems.begin(),
            m_scheduleItems.end(),
            [](const ScheduleItem& s) { return s.numberOfFrames <= 0; }),
        m_scheduleItems.end());
}

} // namespace amaze
} // namespace marengo