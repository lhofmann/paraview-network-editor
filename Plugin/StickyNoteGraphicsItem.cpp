#include "StickyNoteGraphicsItem.h"

#include <pqPipelineSource.h>
#include <pqUndoStack.h>
#include <vtkSMProxy.h>
#include <vtkSMPropertyHelper.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

namespace ParaViewNetworkEditor {

StickyNoteGraphicsItem::StickyNoteGraphicsItem(pqPipelineSource* source) {
  this->source_ = source;
  setZValue(STICKYNOTEGRAPHICSITEM_DEPTH);
  this->setAcceptHoverEvents(true);
  this->loadSize();
  this->updateHandles();
}

StickyNoteGraphicsItem::~StickyNoteGraphicsItem() = default;

void StickyNoteGraphicsItem::paint(QPainter *p, const QStyleOptionGraphicsItem *options, QWidget *widget) {
  if (!source_)
    return;
  bool modified = source_->modifiedState() != pqProxy::UNMODIFIED;
  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);

  p->fillRect(this->rect(), QColor(255, 255, 255, 128));

  p->restore();
  p->save();

  QColor backgroundColor("#3b3d3d");
  if (auto proxy = source_->getProxy()) {
    double r = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(0);
    double g = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(1);
    double b = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(2);
    backgroundColor.setRgbF(r, g, b);
  }

  QRectF rect = this->rect();
  rect.adjust(0, 25, 0, 0);
  p->fillRect(rect, backgroundColor);

  QColor borderColor("#282828");
  if (modified) {
    borderColor = QColor("#FBBC05");
  } else if (isSelected()) {
    borderColor = QColor("#dd0308");
  }
  p->setBrush(QColor(0,0,0,0));
  p->setPen(QPen(QBrush(borderColor), 1.0));
  if (modified || isSelected()) {
    p->drawRect(this->rect());
  }
  p->restore();
}

void StickyNoteGraphicsItem::showToolTip(QGraphicsSceneHelpEvent *e) {
  if (!source_)
    return;
  QString caption, text;
  if (auto proxy = source_->getProxy()) {
    caption = vtkSMPropertyHelper(proxy, "Caption").GetAsString();
    text = vtkSMPropertyHelper(proxy, "Text").GetAsString();
  }
  QString s;
  s += "<html><body><table style=\"white-space: nowrap;\">";
  s += "<tr><td>" + caption + "</td></tr>";
  s += "<tr><td>" + text + "</td></tr>";
  s += "</table></body></html>";
  this->showToolTipHelper(e, s);
}

const float handleSize = 5.;

void StickyNoteGraphicsItem::updateHandles() {
  QRectF b = this->rect();
  const float s = handleSize;
  handles_[HANDLE_BOTTOM_RIGHT] = QRectF(b.right() - s, b.bottom() - s, s, s);
  handles_[HANDLE_BOTTOM] = QRectF(b.left(), b.bottom() - s, b.width(), s);
  handles_[HANDLE_RIGHT] = QRectF(b.right() - s, b.top() - s, s, b.height());
}

StickyNoteGraphicsItem::Handle StickyNoteGraphicsItem::handleAt(const QPointF& pos) {
  if (handles_[HANDLE_BOTTOM_RIGHT].contains(pos)) {
    return HANDLE_BOTTOM_RIGHT;
  } else if (handles_[HANDLE_BOTTOM].contains(pos)) {
    return HANDLE_BOTTOM;
  } else if (handles_[HANDLE_RIGHT].contains(pos)) {
    return HANDLE_RIGHT;
  } else {
    return HANDLE_NONE;
  }
}

const QCursor StickyNoteGraphicsItem::handle_cursors_[4] {
  Qt::SizeVerCursor,   // BOTTOM
  Qt::SizeHorCursor,   // HANDLE_RIGHT
  Qt::SizeFDiagCursor, // HANDLE_BOTTOM_RIGHT
  Qt::ArrowCursor
};

void StickyNoteGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (this->handle_selected_ != HANDLE_NONE) {
    QPointF mouse_pos = event->pos();
    this->prepareGeometryChange();
    QRectF rect = this->rect();

    if (this->handle_selected_ == HANDLE_BOTTOM_RIGHT) {
      rect.setRight( this->mouse_press_rect_.right() + mouse_pos.x() - this->mouse_press_pos_.x());
      rect.setBottom(this->mouse_press_rect_.bottom() + mouse_pos.y() - this->mouse_press_pos_.y());
    } else if (this->handle_selected_ == HANDLE_BOTTOM) {
      rect.setBottom(this->mouse_press_rect_.bottom() + mouse_pos.y() - this->mouse_press_pos_.y());
    } else if (this->handle_selected_ == HANDLE_RIGHT) {
      rect.setRight( this->mouse_press_rect_.right() + mouse_pos.x() - this->mouse_press_pos_.x());
    }

    this->setRect(rect);
    this->updateHandles();
  } else {
    SourceGraphicsItem::mouseMoveEvent(event);
  }
}

void StickyNoteGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  this->handle_selected_ = handleAt(event->pos());
  if (this->handle_selected_ != HANDLE_NONE) {
    this->mouse_press_pos_ = event->pos();
    this->mouse_press_rect_ = this->rect();
  } else {
    SourceGraphicsItem::mousePressEvent(event);
  }
}

void StickyNoteGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (this->handle_selected_ != HANDLE_NONE) {
    this->handle_selected_ = HANDLE_NONE;
    this->update();
    this->prepareGeometryChange();
    QRectF rect = this->rect();
    float nw = (std::round(rect.width() / 25.f)) * 25.f;
    float nh = (std::round(rect.height() / 25.f)) * 25.f;
    if (nw < 25.f) {
      nw = 25.f;
    }
    if (nh < 25.f) {
      nh = 25.f;
    }
    rect.setWidth(nw);
    rect.setHeight(nh);
    this->setRect(rect);
    this->updateHandles();
    // this->storePosition();
    if (source_) {
      if (auto proxy = source_->getProxy()) {
        BEGIN_UNDO_SET("Resize Sticky Note");
        QRectF rect = this->rect();
        proxy->SetAnnotation("Node.width", std::to_string(rect.width()).c_str());
        proxy->SetAnnotation("Node.height", std::to_string(rect.height()).c_str());
        END_UNDO_SET();
      }
    }
  } else {
    SourceGraphicsItem::mouseReleaseEvent(event);
  }
}

void StickyNoteGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  this->setCursor(Qt::ArrowCursor);
  SourceGraphicsItem::hoverLeaveEvent(event);
}

void StickyNoteGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
  if (isSelected()) {
    Handle handle = handleAt(event->pos());
    this->setCursor(handle_cursors_[handle]);
  }
  SourceGraphicsItem::hoverMoveEvent(event);
}

void StickyNoteGraphicsItem::storePosition() {
  SourceGraphicsItem::storePosition();
}

void StickyNoteGraphicsItem::loadSize() {
  if (!this->source_)
    return;
  if (auto proxy = source_->getProxy()) {
    QRectF rect = this->rect();
    this->prepareGeometryChange();
    try {
      rect.setWidth(std::stof(proxy->GetAnnotation("Node.width")));
      rect.setHeight(std::stof(proxy->GetAnnotation("Node.height")));
    } catch (...) {
    }
    this->setRect(rect);
    this->updateHandles();
  }
}

void StickyNoteGraphicsItem::loadPosition() {
  this->loadSize();
  SourceGraphicsItem::loadPosition();
}


}
