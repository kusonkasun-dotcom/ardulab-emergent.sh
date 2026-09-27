#include "project/Project.h"

namespace ardulab::project {

Project::Project(ProjectMetadata metadata)
    : m_metadata(std::move(metadata))
{
}

const ComponentInstance* Project::findInstance(const core::InstanceId& id) const noexcept
{
    for (const ComponentInstance& instance : m_instances) {
        if (instance.instanceId == id) {
            return &instance;
        }
    }
    return nullptr;
}

bool Project::addInstance(ComponentInstance instance)
{
    if (instance.instanceId.isValid() && findInstance(instance.instanceId) != nullptr) {
        return false;
    }
    m_instances.push_back(std::move(instance));
    m_dirty = true;
    return true;
}

std::size_t Project::unresolvedReferenceCount() const noexcept
{
    std::size_t count = 0;
    for (const ComponentInstance& instance : m_instances) {
        if (instance.referenceState == ReferenceState::Unresolved) {
            ++count;
        }
    }
    return count;
}

} // namespace ardulab::project
