// wxQt 3.3.3 converts negative vertical wheel deltas to horizontal wx events.
// Handle the native Qt wheel event before wxWidgets loses its direction.
// This helper exists only in wxQt builds.
#pragma once

#ifdef __WXQT__

#include <functional>
#include <utility>
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QWidget>

class GenevensisQtWheelFilter final : public QObject {
public:
  GenevensisQtWheelFilter(QWidget* widget, std::function<void(int)> handler)
    : QObject(widget), handler_(std::move(handler)) {
    widget->installEventFilter(this);
    if (auto* area = qobject_cast<QAbstractScrollArea*>(widget)) {
      if (area->viewport()) area->viewport()->installEventFilter(this);
    }
  }

protected:
  bool eventFilter(QObject* watched, QEvent* event) override {
    if (event->type() == QEvent::Wheel) {
      auto* wheel = static_cast<QWheelEvent*>(event);
      const int delta = wheel->angleDelta().y();
      if (delta != 0) {
        handler_(delta);
        wheel->accept();
        return true;
      }
    }
    return QObject::eventFilter(watched, event);
  }

private:
  std::function<void(int)> handler_;
};

template<typename Callback>
void installGenevensisQtWheelFilter(wxWindow* window, Callback&& callback) {
  QWidget* widget = window->GetHandle();
  if (!widget) return;
  // QObject ownership ties the filter to the Qt widget's lifetime.
  new GenevensisQtWheelFilter(
    widget, std::function<void(int)>(std::forward<Callback>(callback)));
}

#endif // __WXQT__
