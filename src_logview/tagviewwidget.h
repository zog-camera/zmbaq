#ifndef TAGVIEWWIDGET_H
#define TAGVIEWWIDGET_H

#include <QWidget>
#include <QVariantMap>
#include <QStandardItemModel>
#include <QMutex>
#include "jsoncpp/json/value.h"
#include "mbytearray.h"
#include "zmbaq_common.h"

class QLabel;
class QLineEdit;
class QTableView;
class QTextEdit;


class TagViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TagViewWidget(const MByteArray& selection_tag,
                           int rows_limit = 2500,
                           QWidget *parent = 0);
    virtual ~TagViewWidget();

    const MByteArray& gettag() const;
    
    

signals:

public slots:

    //select by tag:
    void post(const Json::Value& metadata, MByteArrayPtr msg);

private:
    void check_limit_dumpfile();
    void dump_file();

    QMutex mut;
    MByteArray tag;
    QLabel* queryLabel;
    QLineEdit* queryEdit;
    QTextEdit* textEdit;
    int d_max_rows;
    int rows_cnt;

//    static const int time_col_width = 24;
//    MByteArray time_col_text;
};

#endif // TAGVIEWWIDGET_H
