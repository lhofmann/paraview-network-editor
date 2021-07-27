#include "StickyNoteGraphicsItem.h"

#include <pqPipelineSource.h>
#include <pqUndoStack.h>
#include <vtkSMProxy.h>
#include <vtkSMPropertyHelper.h>

#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QTextDocument>
#include <QWidget>

namespace ParaViewNetworkEditor {

StickyNoteGraphicsItem::StickyNoteGraphicsItem(pqPipelineSource* source) {
  this->handle_cursors_ = {
    Qt::SizeVerCursor,   // BOTTOM
    Qt::SizeHorCursor,   // HANDLE_RIGHT
    Qt::SizeFDiagCursor, // HANDLE_BOTTOM_RIGHT
    Qt::ArrowCursor
  };

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
  QColor backgroundColor("#3b3d3d");
  QColor textColor(0, 0, 0);
  float opacity = 1.;
  QString font_family = "Noto Sans";
  bool bold {false}, italic {false};
  int font_size = 10;
  int justification = 0;
  QColor captionBackgroundColor("#3b3d3d");
  QColor captionTextColor(0, 0, 0);
  float captionOpacity = 1.;
  QString caption_font_family = "Noto Sans";
  bool caption_bold {false}, caption_italic {false};
  int caption_font_size = 13;
  int caption_justification = 0;
  QString caption, text;
  bool html = false;
  if (auto proxy = source_->getProxy()) {
    double r = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(0);
    double g = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(1);
    double b = vtkSMPropertyHelper(proxy, "BackgroundColor").GetAsDouble(2);
    backgroundColor.setRgbF(r, g, b);
    r = vtkSMPropertyHelper(proxy, "TextColor").GetAsDouble(0);
    g = vtkSMPropertyHelper(proxy, "TextColor").GetAsDouble(1);
    b = vtkSMPropertyHelper(proxy, "TextColor").GetAsDouble(2);
    textColor.setRgbF(r, g, b);
    opacity = vtkSMPropertyHelper(proxy, "Opacity").GetAsDouble();
    backgroundColor.setAlphaF(opacity);
    font_family = vtkSMPropertyHelper(proxy, "FontFamily").GetAsString();
    bold = vtkSMPropertyHelper(proxy, "Bold").GetAsInt();
    italic = vtkSMPropertyHelper(proxy, "Italic").GetAsInt();
    font_size = vtkSMPropertyHelper(proxy, "FontSize").GetAsInt();
    justification = vtkSMPropertyHelper(proxy, "Justification").GetAsInt();

    r = vtkSMPropertyHelper(proxy, "CaptionBackgroundColor").GetAsDouble(0);
    g = vtkSMPropertyHelper(proxy, "CaptionBackgroundColor").GetAsDouble(1);
    b = vtkSMPropertyHelper(proxy, "CaptionBackgroundColor").GetAsDouble(2);
    captionBackgroundColor.setRgbF(r, g, b);
    r = vtkSMPropertyHelper(proxy, "CaptionTextColor").GetAsDouble(0);
    g = vtkSMPropertyHelper(proxy, "CaptionTextColor").GetAsDouble(1);
    b = vtkSMPropertyHelper(proxy, "CaptionTextColor").GetAsDouble(2);
    captionTextColor.setRgbF(r, g, b);
    captionOpacity = vtkSMPropertyHelper(proxy, "CaptionOpacity").GetAsDouble();
    captionBackgroundColor.setAlphaF(captionOpacity);
    caption_font_family = vtkSMPropertyHelper(proxy, "CaptionFontFamily").GetAsString();
    caption_bold = vtkSMPropertyHelper(proxy, "CaptionBold").GetAsInt();
    caption_italic = vtkSMPropertyHelper(proxy, "CaptionItalic").GetAsInt();
    caption_font_size = vtkSMPropertyHelper(proxy, "CaptionFontSize").GetAsInt();
    caption_justification = vtkSMPropertyHelper(proxy, "CaptionJustification").GetAsInt();

    caption = vtkSMPropertyHelper(proxy, "Caption").GetAsString();
    text = vtkSMPropertyHelper(proxy, "Text").GetAsString();
    html = vtkSMPropertyHelper(proxy, "EnableHTML").GetAsInt();
  }

  QFont caption_font(caption_font_family, caption_font_size, caption_bold ? QFont::Bold : QFont::Normal, caption_italic);
  caption_font.setPixelSize(((caption_font_size * 4.) / 3.));
  QFontMetrics fm = QFontMetrics(caption_font);
  QString caption_short = fm.elidedText(caption, Qt::ElideMiddle, this->rect().width() - 8);
  QSize caption_size = fm.size(Qt::TextSingleLine, caption_short);

  const int header_height = caption_size.height();

  p->save();
  p->setRenderHint(QPainter::Antialiasing, true);

  QRectF rect = this->rect();
  rect.setHeight(header_height + 4);
  p->fillRect(rect, captionBackgroundColor);

  p->restore();
  p->save();

  rect = this->rect();
  rect.adjust(0, header_height + 4, 0, 0);
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

  p->save();

  p->setFont(caption_font);
  p->setPen(captionTextColor);

  rect = this->rect();
  rect.adjust(4, 2, 4, 0);
  rect.setHeight(header_height - 2);

  if (caption_justification == 1) { // center
    rect.setLeft(0.5 * (rect.left() + rect.right()) - caption_size.width() / 2.);
  } else if (caption_justification == 2) { // right
    rect.setLeft(rect.right() - caption_size.width() - 4);
  }

  p->drawText(rect, caption_short);

  if (this->rect().height() > header_height) {
    QTextDocument td;
    QTextOption textOption;
    textOption.setAlignment((justification == 1) ? Qt::AlignCenter : ((justification == 2) ? Qt::AlignRight : Qt::AlignLeft));
    textOption.setWrapMode(QTextOption::WordWrap);
    td.setDefaultTextOption(textOption);

    QFont contentFont(font_family, font_size, bold ? QFont::Bold : QFont::Normal, italic);
    contentFont.setPixelSize(((font_size * 4.) / 3.));
    td.setDefaultFont(contentFont);
    if (html) {
      td.setHtml(text);
    } else {
      td.setPlainText(text);
    }
    rect = this->rect();
    rect.adjust(2, header_height + 4, 2, 2);
    rect.setHeight(rect.height() - 2);
    rect.setWidth(rect.width() - 2);
    p->translate(rect.topLeft());
    td.setTextWidth(rect.width());
    td.setPageSize(rect.size());

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, textColor);
    if (rect.isValid()) {
      p->setClipRect(rect.translated(-rect.topLeft()));
      ctx.clip = rect.translated(-rect.topLeft());
    }
    td.documentLayout()->draw(p, ctx);
  }

  p->restore();
}

void StickyNoteGraphicsItem::showToolTip(QGraphicsSceneHelpEvent *e) {
  if (!source_)
    return;
  QString caption, text;
  bool html = false;
  if (auto proxy = source_->getProxy()) {
    caption = vtkSMPropertyHelper(proxy, "Caption").GetAsString();
    text = vtkSMPropertyHelper(proxy, "Text").GetAsString();
    html = vtkSMPropertyHelper(proxy, "EnableHTML").GetAsInt();
  }
  QString s;
  s += "<html><body><table width=\"300px\">";
  s += "<tr><td>" + caption + "</td></tr>";
  s += "<tr><td>" + (html ? text : text.toHtmlEscaped().replace("\n", "<br/>")) + "</td></tr>";
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
