#ifndef NAVIGATION_HISTORY_HPP
#define NAVIGATION_HISTORY_HPP

#include <memory>

#include <QObject>
#include <QtQml/QQmlPropertyMap>

class NavigationHistory : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(bool previousEmpty READ isPreviousEmpty NOTIFY previousEmptyChanged FINAL)
    /**
     * true when there is no page to go forward to
     */
    Q_PROPERTY(bool nextEmpty READ isNextEmpty NOTIFY nextEmptyChanged FINAL)
    /**
     * current path
     */
    Q_PROPERTY(QStringList viewPath READ viewPath NOTIFY viewPathChanged FINAL)
    /**
     * properties of the current view,
     * * properties that are pushed will be accessible thru this property
     * * views may store values in this property, they will be restored when navigating back
     */
    Q_PROPERTY(QQmlPropertyMap* viewProp READ viewProp NOTIFY viewPropChanged FINAL)

    enum class PostAction{
        Stay,
        Go
    };
    Q_ENUM(PostAction)

public:
    /// tells whether the properties of a history node are still usable
    using NodeValidator = bool (*)(const QQmlPropertyMap&);

    explicit NavigationHistory(NodeValidator isNodeValid, QObject *parent = nullptr);

    QVariant getCurrent();
    bool isPreviousEmpty();
    bool isNextEmpty();
    QStringList viewPath() const;
    QQmlPropertyMap* viewProp() const;

    Q_INVOKABLE bool match(const QStringList& path,  const QStringList& pattern);
    Q_INVOKABLE bool exactMatch(const QStringList& path,  const QStringList& pattern);

signals:
    void navigate(Qt::FocusReason);
    void previousEmptyChanged(bool empty);
    void nextEmptyChanged(bool empty);
    void viewPathChanged(const QStringList& viewPath);
    void viewPropChanged(QQmlPropertyMap*);

public slots:
    /**
     *
     * navigate to a new page
     *
     * @param path: path of the page to load
     * @param properties: values that will be set to the current viewProp
     *
     * example:
     * \code
     * //push the view foo, then bar, set baz to plop in the view "bar"
     *  push(["foo", "bar"], {baz: "plop"})
     * \endcode
     */
    Q_INVOKABLE void push(QStringList path, const QVariantMap& properties, Qt::FocusReason focusReason = Qt::OtherFocusReason);
    Q_INVOKABLE void push(QStringList path, Qt::FocusReason focusReason = Qt::OtherFocusReason);



    /**
     * @brief modify the current history path
     *
     * @note:
     * * invoking update won't cause page to be reloaded, this mainly affects history
     * * invoking update when there is no path will create a node (the initial history node)
     */
    Q_INVOKABLE void update(QStringList path);

    Q_INVOKABLE void update(QStringList path, const QVariantMap& properties);


    /// Go to previous page, the page we leave can be reached again with next()
    void previous(Qt::FocusReason = Qt::OtherFocusReason);

    /// Go back to the page left by previous(), pushing a new page forgets it
    void next(Qt::FocusReason = Qt::OtherFocusReason);

private:
    void updateCurrent();
    void clearForward();

    /// pages kept for next(), the oldest ones are dropped first
    static constexpr size_t MAX_FORWARD_SIZE = 50;

    NodeValidator m_isNodeValid = nullptr;
    std::vector<std::pair<QStringList, std::unique_ptr<QQmlPropertyMap>>> m_history;
    std::vector<std::pair<QStringList, std::unique_ptr<QQmlPropertyMap>>> m_forward;
    QStringList m_viewPath;
    QQmlPropertyMap* m_viewProperties = nullptr;
};

#endif // NAVIGATION_HISTORY_HPP
