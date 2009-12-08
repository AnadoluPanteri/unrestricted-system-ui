#ifndef CALLHANDLER_H
#define CALLHANDLER_H

#include <QObject>
#ifdef UNIT_TEST
    #include "../tests/ut_pincodequerybusinesslogic/pinstubs.h"
#else
    #include <call-ui/CallUiServiceApi>
#endif

using namespace CallUi;

/*!
 * Class to handle the call action for emergency call
 */
class CallHandler : public QObject
{
    Q_OBJECT

public:
    CallHandler(QObject *parent);
    ~CallHandler();

    bool callOngoing();
    static inline const char* envVar() { return "PIN_EM_CALL"; }
    static void debugPrintCallNum(const char *funcInfo);

signals:
    void callStarted();
    void callDone();

public slots:
    bool startCall();

private slots:
    void callStarted(CallUi::PendingCallRequest *req);
    void callDone(QString uid, int reason, int duration, QString message);

private:
    CallUiServiceApi *callUi;
    QString uid;
    bool calling;

#ifdef UNIT_TEST
   friend class Ut_PinCodeQueryBusinessLogic;
#endif // UNIT_TEST
};

#endif // CALLHANDLER_H
