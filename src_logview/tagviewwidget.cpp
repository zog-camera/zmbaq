#include "tagviewwidget.h"
#include <QHBoxLayout>
#include <QJsonObject>
#include <QApplication>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QHeaderView>
#include <QLabel>
#include <QTextEdit>
#include <QMutexLocker>
#include "cds_map.h"
#include "loggersettings.h"
#include <QTextStream>
#include "logsgrabber.h"


TagViewWidget::TagViewWidget(const MByteArray& selection_tag,
                             int rows_limit,
                             QWidget *parent)
    : QWidget(parent), tag(selection_tag)
{
    d_max_rows = rows_limit;
    queryLabel = new QLabel(QApplication::translate("nestedlayouts", "Query:"), this);
    queryEdit = new QLineEdit(this);
    textEdit = new QTextEdit(this);
    textEdit->setWordWrapMode(QTextOption::WrapMode::NoWrap);

    QHBoxLayout *queryLayout = new QHBoxLayout();
    queryLayout->addWidget(queryLabel);
    queryLayout->addWidget(queryEdit);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(queryLayout);
    mainLayout->addWidget(textEdit);
    setLayout(mainLayout);

//    time_col_text.resize(time_col_width);
//    time_col_text.fill(QChar(' '));
    rows_cnt = 0;

}

TagViewWidget::~TagViewWidget()
{
    dump_file();
}

const MByteArray& TagViewWidget::gettag() const
{
   return tag;
}

void TagViewWidget::dump_file()
{
    auto fp = LoggerSettings::instance()->make_logfile(tag);
    QTextStream out((QIODevice*)fp.get());
    out << textEdit->toPlainText();
}

void TagViewWidget::check_limit_dumpfile()
{
    if (rows_cnt > d_max_rows)
    {
        dump_file();
        textEdit->clear();
        rows_cnt = 0;
    }
}
MByteArray make_date_time(const QJsonObject& jo)
{
    MByteArray tm;
    tm += jo["date"].toString();
    tm += " ";
    tm += jo["timezone"].toString();
    tm += " ";
    tm += jo["time"].toString();
    tm += " ";
    tm += jo["timesec"].toString();
    tm += " ";
    return tm;
}

MByteArray make_date_time(const Json::Value& jo)
{
    MByteArray tm;
    tm += jo["date"].asCString();
    tm += " ";
    tm += jo["timezone"].asCString();
    tm += " ";
    tm += jo["time"].asCString();
    tm += " ";
    tm += jo["timesec"].asCString();
    tm += " ";
    return tm;
}



void TagViewWidget::post(const Json::Value& metadata, MByteArrayPtr msg)
{
    QMutexLocker lk(&mut);
    if (nullptr != metadata.find(CW_KW_OBJECT.begin(), CW_KW_OBJECT.end())
            && ZConstString(CW_KW_OBJECT, &metadata) == tag)
    {
        check_limit_dumpfile();
        ++rows_cnt;
        textEdit->append(make_qlatin1_depend(make_date_time(metadata)));
        textEdit->append(QLatin1String(msg->data(), msg->size()));
        textEdit->append("\n-----------\n");
    }

}

