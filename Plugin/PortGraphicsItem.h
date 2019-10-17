#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_PORTGRAPHICSITEM_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_PORTGRAPHICSITEM_H_

#include "EditorGraphicsItem.h"

#include <QEvent>
#include <QPointF>
#include <QColor>

class pqPipelineFilter;
class pqPipelineSource;

class ConnectionGraphicsItem;
class SourceGraphicsItem;
class PortGraphicsItem;

class PortConnectionIndicator : public EditorGraphicsItem {
public:
    PortConnectionIndicator(PortGraphicsItem* parent, bool up, QColor color);
    virtual ~PortConnectionIndicator() = default;

protected:
    void paint(QPainter* p, const QStyleOptionGraphicsItem* options, QWidget* widget) override;

private:
    PortGraphicsItem* portConnectionItem_;
    bool up_;
    QColor color_;
};

class PortGraphicsItem : public EditorGraphicsItem {
public:
    PortGraphicsItem(SourceGraphicsItem* parent, const QPointF& pos, bool up, QColor color);
    virtual ~PortGraphicsItem();

    void addConnection(ConnectionGraphicsItem* connection);
    void removeConnection(ConnectionGraphicsItem* connection);
    std::vector<ConnectionGraphicsItem*>& getConnections();
    SourceGraphicsItem* getSource();
    virtual void showToolTip(QGraphicsSceneHelpEvent* e) override = 0;

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    virtual void updateConnectionPositions() = 0;

    std::vector<ConnectionGraphicsItem*> connections_;
    SourceGraphicsItem* source_;
    PortConnectionIndicator* connectionIndicator_;
    float size_;
    float lineWidth_;
};

class InputPortGraphicsItem : public PortGraphicsItem {
public:
    InputPortGraphicsItem(SourceGraphicsItem* parent, const QPointF& pos, pqPipelineFilter* source, int port_id);
    virtual ~InputPortGraphicsItem() = default;

    // override for qgraphicsitem_cast (refer qt documentation)
    enum { Type = UserType + InputPortGraphicsType };
    virtual int type() const override { return Type; }

    virtual void showToolTip(QGraphicsSceneHelpEvent* e) override;

protected:
    virtual void paint(QPainter* p, const QStyleOptionGraphicsItem* options,
                       QWidget* widget) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e) override;
    virtual void updateConnectionPositions() override;
};

class OutputPortGraphicsItem : public PortGraphicsItem {
public:
    OutputPortGraphicsItem(SourceGraphicsItem* parent, const QPointF& pos, pqPipelineSource* source, int port_id);
    virtual ~OutputPortGraphicsItem() = default;

    // override for qgraphicsitem_cast (refer qt documentation)
    enum { Type = UserType + OutputPortGraphicsType };
    virtual int type() const override { return Type; }

    virtual void showToolTip(QGraphicsSceneHelpEvent* e) override;

protected:
    virtual void paint(QPainter* p, const QStyleOptionGraphicsItem* options,
                       QWidget* widget) override;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent* e) override;

    virtual void updateConnectionPositions() override;
};


#endif //PARAVIEWNETWORKEDITOR_PLUGIN_PORTGRAPHICSITEM_H_
