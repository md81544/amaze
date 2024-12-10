#include "scheduler.h"

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
    for (auto& si : m_scheduleItems) {
        --si.numberOfFrames;
        if (si.numberOfFrames <= 0) {
            if (si.doAfter) {
                si.doAfter();
            }
        } else if (si.doWhile) {
            si.doWhile();
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