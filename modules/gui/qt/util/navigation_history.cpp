#include "navigation_history.hpp"
#include <cassert>

NavigationHistory::NavigationHistory(NodeValidator isNodeValid, QObject *parent)
    : QObject(parent)
    , m_isNodeValid(isNodeValid)
{
    assert(m_isNodeValid);
}

bool NavigationHistory::isPreviousEmpty()
{
    return m_history.size() <= 1;
}

bool NavigationHistory::isNextEmpty()
{
    return m_forward.empty();
}

void NavigationHistory::push(QStringList path, const QVariantMap& properties, Qt::FocusReason focusReason)
{
    auto prop = std::make_unique<QQmlPropertyMap>();
    for (auto it =  properties.keyValueBegin(); it != properties.keyValueEnd(); ++it)
        prop->insert((*it).first, (*it).second);

    clearForward();
    m_history.emplace_back(path, std::move(prop));

    updateCurrent();

    if (m_history.size() == 2)
        emit previousEmptyChanged(false);

    emit navigate(focusReason);
}

void NavigationHistory::push(QStringList path, Qt::FocusReason focusReason)
{
    clearForward();
    m_history.emplace_back(path, std::make_unique<QQmlPropertyMap>());

    updateCurrent();

    if (m_history.size() == 2)
        emit previousEmptyChanged(false);

    emit navigate(focusReason);

}

void NavigationHistory::update(QStringList path)
{
    if (m_history.size() == 0)
    {
        m_history.emplace_back(path, std::make_unique<QQmlPropertyMap>());
    }
    else
    {
        auto& last = *m_history.rbegin();
        last.first = path;
    }
    updateCurrent();
}

void NavigationHistory::update(QStringList path, const QVariantMap& properties)
{
    if (m_history.size() == 0)
    {
        auto prop = std::make_unique<QQmlPropertyMap>();
        for (auto it =  properties.keyValueBegin(); it != properties.keyValueEnd(); ++it)
        {
            prop->insert((*it).first, (*it).second);
        }
        m_history.emplace_back(path, std::move(prop));
    }
    else
    {
        auto& last = *m_history.rbegin();
        last.first = path;
    }
    updateCurrent();
}


void NavigationHistory::previous(Qt::FocusReason reason)
{
    if (m_history.size() <= 1)
        return;

    const bool wasNextEmpty = isNextEmpty();

    //keep the page we leave, so that next() can come back to it
    m_forward.push_back(std::move(m_history.back()));
    m_history.pop_back();
    if (m_forward.size() > MAX_FORWARD_SIZE)
        m_forward.erase(m_forward.begin());

    while (m_history.size() > 1 && !m_isNodeValid(*m_history.back().second))
        m_history.pop_back();

    if (m_history.size() == 1)
        emit previousEmptyChanged(true);
    if (wasNextEmpty)
        emit nextEmptyChanged(false);

    updateCurrent();
    emit navigate(reason);
}

void NavigationHistory::next(Qt::FocusReason reason)
{
    const bool wasNextEmpty = isNextEmpty();

    //pages may have become invalid since we left them
    while (!m_forward.empty() && !m_isNodeValid(*m_forward.back().second))
        m_forward.pop_back();

    if (m_forward.empty())
    {
        if (!wasNextEmpty)
            emit nextEmptyChanged(true);
        return;
    }

    const bool wasPreviousEmpty = isPreviousEmpty();

    m_history.push_back(std::move(m_forward.back()));
    m_forward.pop_back();

    if (wasPreviousEmpty)
        emit previousEmptyChanged(false);
    if (m_forward.empty())
        emit nextEmptyChanged(true);

    updateCurrent();
    emit navigate(reason);
}

void NavigationHistory::clearForward()
{
    if (m_forward.empty())
        return;
    m_forward.clear();
    emit nextEmptyChanged(true);
}

void NavigationHistory::updateCurrent()
{
    assert(m_history.size() >= 1);
    const auto it = m_history.rbegin();

    m_viewPath = it->first;
    m_viewProperties = it->second.get();
    emit viewPathChanged(m_viewPath);
    emit viewPropChanged(m_viewProperties);
}

QStringList NavigationHistory::viewPath() const
{
    return m_viewPath;
}

QQmlPropertyMap* NavigationHistory::viewProp() const
{
    return m_viewProperties;
}

Q_INVOKABLE bool NavigationHistory::match(const QStringList& path,  const QStringList& pattern)
{
    if (pattern.length() > path.length())
        return false;
    for (qsizetype i = 0; i < pattern.length(); i++)
    {
        if (pattern[i] != path[i])
            return false;
    }
    return true;
}

Q_INVOKABLE bool NavigationHistory::exactMatch(const QStringList& path,  const QStringList& pattern)
{
    if (pattern.length() != path.length())
        return false;
    for (qsizetype i = 0; i < pattern.length(); i++)
    {
        if (pattern[i] != path[i])
            return false;
    }
    return true;
}
