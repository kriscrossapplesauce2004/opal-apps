#pragma once
#include <QObject>
#include <QQmlEngine>

// Chooses the posture for the whole app: pointer, touch or couch. Width classes are separate and live in AppWindow.
class Posture : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(Mode mode READ mode NOTIFY changed)
    Q_PROPERTY(QString name READ name NOTIFY changed)
    Q_PROPERTY(bool pointer READ pointer NOTIFY changed)
    Q_PROPERTY(bool touch READ touch NOTIFY changed)
    Q_PROPERTY(bool couch READ couch NOTIFY changed)
    // A controller was the last input; show the focus ring and the hint bar.
    Q_PROPERTY(bool controllerActive READ controllerActive NOTIFY changed)
    // Game mode, gamescope, a handheld or a TV-sized screen.
    Q_PROPERTY(bool couchContext READ couchContext NOTIFY changed)
    // The app opts out of couch (Terminal, Text Editor); couch context then gives touch or pointer.
    Q_PROPERTY(bool allowCouch READ allowCouch WRITE setAllowCouch NOTIFY changed)
    // Set by tests and prototypes: pointer, touch or couch. Empty means automatic.
    Q_PROPERTY(QString forced READ forced WRITE setForced NOTIFY changed)
public:
    enum Mode { Pointer, Touch, Couch };
    Q_ENUM(Mode)

    explicit Posture(QObject *parent = nullptr);
    static Posture *instance();
    static Posture *create(QQmlEngine *, QJSEngine *);

    Mode mode() const { return m_mode; }
    QString name() const;
    bool pointer() const { return m_mode == Pointer; }
    bool touch() const { return m_mode == Touch; }
    bool couch() const { return m_mode == Couch; }
    bool controllerActive() const { return m_controller; }
    bool couchContext() const { return m_couchContext; }
    bool allowCouch() const { return m_allowCouch; }
    void setAllowCouch(bool allow);
    QString forced() const { return m_forced; }
    void setForced(const QString &value);

signals:
    void changed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void detectContext();
    void update();

    Mode m_mode = Pointer;
    Mode m_lastHand = Pointer;
    bool m_controller = false;
    bool m_couchContext = false;
    bool m_handheld = false;
    bool m_allowCouch = true;
    QString m_forced;
};
