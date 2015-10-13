#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QWidget>
#include <QMap>
#include <QMutex>
#include <QVariantMap>
#include "jsoncpp/json/value.h"
#include "zmbaq_common.h"
#include "cds_map.h"
#include "logsgrabber.h"

class QLineEdit;
class QLabel;
class QTabWidget;
class TagViewWidget;


//Hash<QByteArray, SHP(SurvlObject)>
typedef LCDS::MapHnd<MByteArray, SHP(TagViewWidget)> HashViewers;


class CentralWidget : public QWidget,
        public std::enable_shared_from_this<CentralWidget>
{
    Q_OBJECT
public:
    explicit CentralWidget(QWidget *parent = 0);
    virtual ~CentralWidget();


    /** Get or crete tag viewer,
     * emits sig_tab_created() if new tab is created. */
    SHP(TagViewWidget) get_tab(ZConstString tag);


signals:
    void sig_tab_created(SHP(TagViewWidget));

public slots:
    //on address enter:
    void on_enter();

    void pass_msg(const Json::Value& metadata, MByteArrayPtr msg);

private:

    MByteArray temp;
    QTabWidget* tabs;
    SHP(HashViewers) tagviews;
    LeechLair<CentralWidget, LogsGrabber, float/*fps*/> udp_lair;

    QLineEdit* edit;
    QLabel* label;


};

#endif // CENTRALWIDGET_H
