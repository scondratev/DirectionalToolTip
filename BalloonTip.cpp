#include "BalloonTip.h"
#include <QBitmap>
#include <QPainter>
#include <QMouseEvent>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QGridLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QPainterPath>
#include <QMessageBox>

//#include <QWidgetTextControl>


static BalloonTip *theSolitaryBalloonTip = 0;

void BalloonTip::showBalloon(QMessageBox::Icon icon, const QString& title,
                              const QString& message,
                              const QPoint& pos, int timeout, bool showArrow, int arrowDir)
{
    hideBalloon();
    if (message.isEmpty() && title.isEmpty())
        return;

    theSolitaryBalloonTip = new BalloonTip(icon, title, message);
    if (timeout < 0)
        timeout = 10000; //10 s default
    theSolitaryBalloonTip->balloon(pos, timeout, showArrow, arrowDir);
}

void BalloonTip::hideBalloon()
{
    if (!theSolitaryBalloonTip)
        return;
    theSolitaryBalloonTip->hide();
    delete theSolitaryBalloonTip;
    theSolitaryBalloonTip = 0;
}

void BalloonTip::updateBalloonPosition(const QPoint& pos)
{
    if (!theSolitaryBalloonTip)
        return;
    theSolitaryBalloonTip->hide();
    theSolitaryBalloonTip->balloon(pos, 0, theSolitaryBalloonTip->showArrow, theSolitaryBalloonTip->arrowDir);
}

bool BalloonTip::isBalloonVisible()
{
    return theSolitaryBalloonTip;
}

BalloonTip::BalloonTip(QMessageBox::Icon icon, const QString& title,
                         const QString& message)
    : QWidget(0, Qt::ToolTip), timerId(-1), eventTimerId(-1), enablePressEvent(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    //QObject::connect(ti, SIGNAL(destroyed()), this, SLOT(close()));

    QLabel *titleLabel = new QLabel;
    titleLabel->installEventFilter(this);
    titleLabel->setText(title);
    QFont f = titleLabel->font();
    f.setBold(true);
#ifdef Q_OS_WINCE
    f.setPointSize(f.pointSize() - 2);
#endif
    titleLabel->setFont(f);
    titleLabel->setTextFormat(Qt::PlainText); // to maintain compat with windows

#ifdef Q_OS_WINCE
    const int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
    const int closeButtonSize = style()->pixelMetric(QStyle::PM_SmallIconSize) - 2;
#else
    const int iconSize = 18;
    const int closeButtonSize = 15;
#endif

    /*QPushButton *closeButton = new QPushButton;
    closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    closeButton->setIconSize(QSize(closeButtonSize, closeButtonSize));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeButton->setFixedSize(closeButtonSize, closeButtonSize);
    QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));*/

    QLabel *msgLabel = new QLabel;
#ifdef Q_OS_WINCE
    f.setBold(false);
    msgLabel->setFont(f);
#endif
    msgLabel->installEventFilter(this);
    msgLabel->setText(message);
    msgLabel->setTextFormat(Qt::PlainText);
    msgLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // smart size for the message label
#ifdef Q_OS_WINCE
    int limit = QApplication::desktop()->availableGeometry(msgLabel).size().width() / 2;
#else
    int limit = QApplication::desktop()->availableGeometry(msgLabel).size().width() / 3;
#endif
    if (msgLabel->sizeHint().width() > limit) {
        msgLabel->setWordWrap(true);
        /*if (msgLabel->sizeHint().width() > limit) {
            msgLabel->d_func()->ensureTextControl();
            if (QWidgetTextControl *control = msgLabel->d_func()->control) {
                QTextOption opt = control->document()->defaultTextOption();
                opt.setWrapMode(QTextOption::WrapAnywhere);
                control->document()->setDefaultTextOption(opt);
            }
        }*/
#ifdef Q_OS_WINCE
        // Make sure that the text isn't wrapped "somewhere" in the balloon widget
        // in the case that we have a long title label.
        setMaximumWidth(limit);
#else
        // Here we allow the text being much smaller than the balloon widget
        // to emulate the weird standard windows behavior.
        msgLabel->setFixedSize(limit, msgLabel->heightForWidth(limit));
#endif
    }

    QIcon si;
    switch (icon) {
    case QMessageBox::Warning:
        si = style()->standardIcon(QStyle::SP_MessageBoxWarning);
        break;
    case QMessageBox::Critical:
        si = style()->standardIcon(QStyle::SP_MessageBoxCritical);
        break;
    case QMessageBox::Information:
        si = style()->standardIcon(QStyle::SP_MessageBoxInformation);
        break;
    case QMessageBox::NoIcon:
    default:
        break;
    }

    QGridLayout *layout = new QGridLayout;
    if (!si.isNull()) {
        QLabel *iconLabel = new QLabel;
        iconLabel->setPixmap(si.pixmap(iconSize, iconSize));
        iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        iconLabel->setMargin(2);
        layout->addWidget(iconLabel, 0, 0);
        layout->addWidget(titleLabel, 0, 1);
    } else {
        layout->addWidget(titleLabel, 0, 0, 1, 2);
    }

    //layout->addWidget(closeButton, 0, 2);
    layout->addWidget(msgLabel, 1, 0, 1, 3);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setMargin(3);
    setLayout(layout);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0xff, 0xff, 0xe1));
    pal.setColor(QPalette::WindowText, Qt::black);
    setPalette(pal);
}

BalloonTip::~BalloonTip()
{
    theSolitaryBalloonTip = 0;
}

void BalloonTip::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(rect(), pixmap);
}

void BalloonTip::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
}

void BalloonTip::balloon(const QPoint& pos, int msecs, bool showArrow, int arrowDir)
{
    enablePressEvent = false;
    this->arrowDir = arrowDir;
    this->showArrow = showArrow;
    QRect scr = QApplication::desktop()->screenGeometry(pos);
    QSize sh = sizeHint();
    const int border = 1;
    const int ah = 18, ao = 18, aw = 18, rc = 7;

    bool arrowAtTop = false, arrowAtLeft = false, arrowAtBottom = false, arrowAtRight = false;
//    bool arrowTopLeft, arrowCenter, arrowBottomRight;

    switch (arrowDir)
    {
        case 11: case 12: case 1: arrowAtTop = true; break;
        case 2: case 3: case 4: arrowAtRight = true; break;
    case 5: case 6: case 7: arrowAtBottom = true; break;
    case 8: case 9: case 10: arrowAtLeft = true; break;
    }



    //bool arrowAtTop =  (pos.y() + sh.height() + ah < scr.height());
    //bool arrowAtLeft = (pos.x() + sh.width() - ao < scr.width());
    setContentsMargins(border + (arrowAtLeft ? ah : 0) + 3,
                       border + (arrowAtTop ? ah : 0) + 2,
                       border + (arrowAtRight ? ah : 0) + 3,
                       border + (arrowAtBottom? ah : 0) + 2);
    updateGeometry();
    sh  = sizeHint();

    int ml = 0, mr = 0, mt = 0, mb = 0;
    QSize sz = sizeHint();
    ml = arrowAtLeft ? ah : 0;
    mt = arrowAtTop ? ah : 0;
    mr = sz.width() - (arrowAtRight ? ah : 0) - 1;
    mb = sz.height() - (arrowAtBottom ? ah : 0) - 1;


    QPainterPath path;
    path.moveTo(ml + rc, mt);
    // draw TOP
    if (arrowAtTop)
    {
        if (arrowDir == 1) {
           path.lineTo(ml + ao, mt);
            path.lineTo(ml + ao, mt - ah);
            path.lineTo(ml + ao + aw, mt);
            move(pos.x() - ao, pos.y());
        }
         else if (arrowDir == 11) {
            path.lineTo(mr - ao - aw, mt);
            path.lineTo(mr - ao, mt - ah);
            path.lineTo(mr - ao, mt);
            move(pos.x() - sh.width() + ao, pos.y());
        }
        else if (arrowDir == 12) {
            int x = (sz.width() - aw) / 2;
            path.lineTo(ml + x, mt);
            path.lineTo(ml + x + aw / 2, mt - ah);
            path.lineTo(ml + x + aw, mt);
            move(pos.x() - x - rc, pos.y());
        }
    }
    path.lineTo(mr - rc, mt);
    path.arcTo(QRect(mr - rc*2, mt, rc*2, rc*2), 90, -90);

    if (arrowAtRight)
    {
        if (arrowDir == 2) {
           path.lineTo(mr, mt + ao);
            path.lineTo(mr + ah, mt + ao);
            path.lineTo(mr, mt + ao + aw);
            move(pos.x() - sh.width(), pos.y() - ao);
        }
         else if (arrowDir == 3) {
            path.lineTo(mr, mb - ao - aw);
            path.lineTo(mr + ah, mb - ao);
            path.lineTo(mr, mb - ao);
            move(pos.x() - sh.width(), pos.y() - sh.height() + ao);
        }
        else if (arrowDir == 4) {
            int x = (sz.height() - aw) / 2;
            path.lineTo(mr, mt + x);
            path.lineTo(mr + ah, mt + x + aw / 2);
            path.lineTo(mr, mt + x + aw);
            move(pos.x() - sh.width(), pos.y() - x - rc);
        }
    }

    path.lineTo(mr, mb - rc);
    path.arcTo(QRect(mr - rc*2, mb - rc*2, rc*2, rc*2), 0, -90);
    if (arrowAtBottom)
    {
        if (arrowDir == 5) {
            path.lineTo(mr - ao, mb);
            path.lineTo(mr - ao, mb + ah);
            path.lineTo(mr - ao - aw, mb);
            move(pos.x() - sh.width() + ao,
                 pos.y() - sh.height());
        } else if (arrowDir == 7) {
            path.lineTo(ao + aw, mb);
            path.lineTo(ao, mb + ah);
            path.lineTo(ao, mb);
            move(pos.x() - ao, pos.y() - sh.height());
        }
        else if (arrowDir == 6) {
            int x = (sz.width() - aw) / 2;
            path.lineTo(mr - x, mb);
            path.lineTo(mr - x - aw / 2, mb + ah);
            path.lineTo(mr - x - aw, mb);
            move(pos.x() - x - rc, pos.y() - sh.height());
        }
    }
    path.lineTo(ml + rc, mb);
    path.arcTo(QRect(ml, mb - rc*2, rc*2, rc*2), -90, -90);

    if (arrowAtLeft)
    {
        if (arrowDir == 8) {
           path.lineTo(ml, mb - ao);
            path.lineTo(ml - ah, mb - ao);
            path.lineTo(ml, mb - ao - aw);
            move(pos.x(), pos.y() - sh.height() + ao);
        }
         else if (arrowDir == 10) {
            path.lineTo(ml, mt + ao + aw);
            path.lineTo(ml - ah, mt + ao);
            path.lineTo(ml, mt + ao);
            move(pos.x(), pos.y() - ao);
        }
        else if (arrowDir == 9) {
            int x = (sz.height() - aw) / 2;
            path.lineTo(ml, mb - x);
             path.lineTo(ml - ah, mb - x - aw /2);
             path.lineTo(ml, mb - x - aw);
            move(pos.x(), pos.y() - x - rc);
        }
    }

    path.lineTo(ml, mt + rc);
    path.arcTo(QRect(ml, mt, rc*2, rc*2), 180, -90);

    // Set the mask
    QBitmap bitmap = QBitmap(sizeHint());
    bitmap.fill(Qt::color0);
    QPainter painter1(&bitmap);
    painter1.setPen(QPen(Qt::color1, border));
    painter1.setBrush(QBrush(Qt::color1));
    painter1.drawPath(path);
    setMask(bitmap);

    // Draw the border
    pixmap = QPixmap(sz);
    QPainter painter2(&pixmap);
    painter2.setPen(QPen(palette().color(QPalette::Window).darker(160), border));
    painter2.setBrush(palette().color(QPalette::Window));
    painter2.drawPath(path);

    if (msecs > 0)
        timerId = startTimer(msecs);

    //eventTimerId = startTimer(500);
    show();
}

void BalloonTip::mousePressEvent(QMouseEvent *e)
{
//    if (enablePressEvent)
        close();
}

void BalloonTip::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == timerId) {
        killTimer(timerId);
        if (!underMouse())
            close();
        return;
    }
//    if (e->timerId() == eventTimerId)
//        enablePressEvent = true;
    QWidget::timerEvent(e);
}
