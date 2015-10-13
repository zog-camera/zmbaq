#include "centralwidget.h"
#include <QTabWidget>
#include "tagviewwidget.h"
#include <QVBoxLayout>
#include <QJsonObject>
#include <QMutexLocker>
#include <QLabel>
#include <QLineEdit>

#include <czmq.h>


//---------------
CentralWidget::CentralWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(QSize(400, 400));

    tagviews = std::make_shared<HashViewers>();

    tabs = new QTabWidget(this);
    qRegisterMetaType<SHP(TagViewWidget)>("SHP(TagViewWidget)");
    qRegisterMetaType<Json::Value>("Json::Value");

    QVBoxLayout* vl = new QVBoxLayout(this);
    setLayout(vl);

    QWidget* top = new QWidget(this);
    top->setMaximumSize(QSize(400, 40));
    QHBoxLayout* hl = new QHBoxLayout(top);
    top->setLayout(hl);

    label = new QLabel("Enter ip:port, press Return.", top);
    label->setMaximumSize(QSize(250,40));
    edit = new QLineEdit(top);
    edit->setText("localhost:59000");
    connect(edit, &QLineEdit::returnPressed, this, &CentralWidget::on_enter);
    hl->addWidget(label);
    hl->addWidget(edit);

    vl->addWidget(top);
    vl->addWidget(tabs);
}

CentralWidget::~CentralWidget()
{
    //wait for threads:
    udp_lair.clear(true);
    auto mp = tagviews->m();
    HashViewers::map_guard* mg = (HashViewers::map_guard*)mp.get();
    mg->clear();
}

void CentralWidget::pass_msg(const Json::Value& metadata, MByteArrayPtr msg)
{
   if (metadata.type() == Json::objectValue && nullptr != metadata.find(CW_KW_OBJECT.begin(), CW_KW_OBJECT.end()))
   {
       SHP(TagViewWidget) widget = get_tab(ZConstString(CW_KW_OBJECT, &metadata));
       widget->post(metadata, msg);
   }
}

SHP(TagViewWidget) CentralWidget::get_tab(ZConstString tag)
{
    SHP(TagViewWidget) v;
    MByteArray copy(tag.begin(), 0, tag.len);
    auto iter = tagviews->find(copy);
    if (iter.is_end())
    {
        v = std::make_shared<TagViewWidget>(copy);
        tabs->addTab(v.get(), QLatin1String(tag.begin(), tag.size()));
        iter.prepend(copy, v);
        emit sig_tab_created(v);
    }
    else
    {
        v = iter.value();
    }
    return v;
}

void CentralWidget::on_enter()
{
    MByteArray addr = edit->text();
    //split string of type 127.0.0.1:59000
    auto str_list = addr.split(':');
    if (str_list->size() == 2)
    {

        auto iter = str_list->begin();
        const MByteArray& ip = *(iter);
        ++iter;
        quint16 port = (*iter).toInt();

        auto worker_pair = udp_lair.new_leech_with_thread_sz(shared_from_this(), 256 * 1024, 40/*fps*/);
        SHP(LogsGrabber) udp = worker_pair.first;
        connect(udp.get(), &LogsGrabber::message,
                this, &CentralWidget::pass_msg, Qt::QueuedConnection);

        QMetaObject::invokeMethod(udp.get(), "listen",Qt::QueuedConnection,
                                  Q_ARG(MByteArray, ip), Q_ARG(quint16, port));
    }

}
