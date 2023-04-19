#ifndef __PROGRESSDLG_H
#define __PROGRESSDLG_H

#include <QDialog>
#include <memory>
#include <QDir>
#include <QFileInfo>
class QTreeWidgetItem;
class QDir;
class QFileInfo;
namespace Ui {class CProgressDlg;};

class CProgressDlg : public QDialog
{
    Q_OBJECT
public:
    CProgressDlg( QWidget * parent );
    CProgressDlg( const QString & cancelText, QWidget * parent );
    ~CProgressDlg();
    virtual void closeEvent(QCloseEvent* event) override;

    void setStatusLabel();

    void setRelToDir(const QDir& relToDir);

    void setMD5Value( int value );
    int md5Value() const;
    void setMD5Range( int min, int max );
    int md5Min() const;
    int md5Max() const;
    void setMD5Format( const QString & format );
    QString md5Format() const;
    void setCurrentMD5Info( Qt::HANDLE threadID, const QFileInfo& fileInfo);
    void setMD5Finished();
    void updateActiveThreads();

    void setCancelText( const QString & cancelText );
    QString cancelText() const;

    bool wasCanceled() const{ return fCanceled; }
public Q_SLOTS:
    void slotCanceled();
Q_SIGNALS:
    void sigCanceled();
private:
    bool fCanceled{ false };
    bool fMD5Finished{ false };

    QDir fRelToDir;
    std::unique_ptr< Ui::CProgressDlg > fImpl;
};
#endif 