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
    explicit TagViewWidget(const std::string& selection_tag,
                           int rows_limit = 2500,
                           QWidget *parent = 0);
    virtual ~TagViewWidget();

    const std::string& gettag() const;
    
    

signals:

public slots:

    //select by tag:
    void post(const Json::Value& metadata, std::stringPtr msg);

private:
    void check_limit_dumpfile();
    void dump_file();

    QMutex mut;
    std::string tag;
    QLabel* queryLabel;
    QLineEdit* queryEdit;
    QTextEdit* textEdit;
    int d_max_rows;
    int rows_cnt;

//    static const int time_col_width = 24;
//    std::string time_col_text;
};

#endif // TAGVIEWWIDGET_H
