#include "ut_pincodequerybusinesslogic.h"
#include "pincodequeryui.h"

#include <DuiApplication>
#include <DuiApplicationWindow>
#include <DuiTextEdit>
#include <DuiButton>
#include <DuiLocale>

#include <QVariant>
#include <QDebug>

#include "notificationtype.h"

using namespace Cellular;

namespace {
    QString SIMCardNotInserted = trid("NOT DOCUMENTED YET" , "SIM card not inserted.");
    QString SIMCardRejected = trid("qtn_cell_sim_rejected" , "SIM card rejected.");

    QString EmergencyCallHeader = trid("qtn_cell_start_emergency_call" , "Start Emergency call?");
    QString HeaderEnterPinCode = trid("qtn_cell_enter_pin_code", "Enter PIN code");
    QString HeaderEnterPinCode1Left = trid("qtn_cell_enter_pin_code_1", "Enter PIN code. 1 attempts remaining.");
    QString HeaderEnterPinCode2Left = trid("qtn_cell_enter_pin_code_2", "Enter PIN code. 2 attempts remaining.");
    QString HeaderEnterPinCodeNew = trid("qtn_cell_enter_new_pin", "Enter new PIN code");
    QString HeaderEnterPinCodeReNew = trid("qtn_cell_reenter_new_pin", "Re-enter new PIN code");
    QString HeaderEnterPukCode = trid("qtn_cell_enter_PUK_code", "Enter PUK code");
    QString HeaderEnterSimLockCode = trid("qtn_cell_enter_unlock_code", "Enter code for unlocking SIM card");

    /* At the moment this contains several error case that may pop up from SIM library.
       It needs to be clarified, how are they going to be reported to end user. */
    QString TechnicalProblem = trid("qtn_cell_general_sim_error", "Error with SIM card. Continue without SIM?");;
    QString PINCodeIncorrectSettings = trid("qtn_cell_incorrect_pin_settings" , "Incorrect PIN code.");

    QString PINCodeIncorrect2AttemptsLeft = trid("qtn_cell_incorrect_pin_2" , "Incorrect PIN code. 2 attempts remaining.");
    QString PINCodeIncorrect1AttemptLeft = trid("qtn_cell_incorrect_pin_1" , "Incorrect PIN code. 1 attempt remaining.");
    QString PINCodeIncorrectPUKCodeRequired = trid("qtn_cell_incorrect_pin_0" , "Incorrect PIN code. SIM locked with PUK code.");
    QString PUKCodeIncorrect = trid("qtn_cell_incorrect_puk" , "Incorrect PUK code.");
    QString PINCodeChanged = trid("qtn_cell_pin_code_changed" , "PIN code changed.");
    QString PINCodesDoNotMatch = trid("qtn_cell_codes_not_match" , "Codes do not match.");

    QString SIMUnlocked = trid("qtn_cell_sim_unlocked" , "SIM card unlocked");
    QString SIMLocked = trid("qtn_cell_sim_lock_notification", "This SIM card can not be used in the device.");
    QString SIMLockIncorrect = trid("qtn_cell_sim_lock_incorrect", "Incorrect unlock code.");
    QString SIMLockTooFast = trid("qtn_cell_too_fast_entry", "Error with unlock code. Trying again might help.");

}

void Ut_PinCodeQueryBusinessLogic::init()
{    
    m_subject = new PinCodeQueryBusinessLogic(this);
    connect(this, SIGNAL(uiButtonReleased()), m_subject, SLOT(uiButtonReleased()));

    m_sim = m_subject->sim;
    m_simSec = m_subject->simSec;
    m_simLock = m_subject->simLock;
    m_dbus = m_subject->dbus;
}

void Ut_PinCodeQueryBusinessLogic::cleanup()
{
    delete m_subject;
    m_subject = NULL;

    m_sim = NULL;
    m_simSec = NULL;
    m_simLock = NULL;
    m_dbus = NULL;
}

DuiApplication *app;
DuiApplicationWindow *win;
void Ut_PinCodeQueryBusinessLogic::initTestCase()
{
    int argc = 1;
    char* app_name = (char*) "./ut_pincodequerybusinesslogic";
    app = new DuiApplication(argc, &app_name);

    win = new DuiApplicationWindow();
    Qt::WindowFlags flags = 0;
        flags |= Qt::FramelessWindowHint;// | Qt::WindowStaysOnTopHint;
    win->setWindowOpacity(0);
    win->setWindowFlags(flags);

    qRegisterMetaType<NotificationType::Type>("NotificationType::Type");
}

void Ut_PinCodeQueryBusinessLogic::cleanupTestCase()
{
    delete app;
}

///////////////// HELPERS ///////////////////

void Ut_PinCodeQueryBusinessLogic::doAndVerifyStateChange(SIM::SIMStatus status)
{
    m_sim->simStatus = status;
    m_sim->emitStatusChanged();
    QCOMPARE(m_subject->previousSimState, (int)(m_sim->simStatus));
}

void Ut_PinCodeQueryBusinessLogic::emitButtonReleased(QString buttonName)
{
    QString tmp = this->objectName();
    this->setObjectName(buttonName);
    emit uiButtonReleased();
    this->setObjectName(tmp);
}

void Ut_PinCodeQueryBusinessLogic::doAndVerifyPinOk()
{
    m_simSec->pin = QString("1234");
    if(SIM::PINRequired != m_subject->previousSimState){
        doAndVerifyStateChange(SIM::PINRequired);
    }
    m_subject->initialized = true;

    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");

    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
}

void Ut_PinCodeQueryBusinessLogic::doAndVerifyPinNok(QString code, QString errorText)
{
    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));

    if(SIM::PINRequired != m_subject->previousSimState){
        this->doAndVerifyStateChange(SIM::PINRequired);
    }
    m_subject->initialized = true;

    m_subject->uiPin->getCodeEntry()->setText(code);
    emitButtonReleased("enterButton");

    QCOMPARE(spy.count(), 1); // make sure the signal was emitted exactly one time
    QList<QVariant> arguments = spy.takeFirst(); // take the first signal
    qDebug() << Q_FUNC_INFO << errorText;
    QVERIFY(arguments.at(0).toString() == errorText);
}

///////////////// TESTS ///////////////////

void Ut_PinCodeQueryBusinessLogic::testStateChange()
{
    QCOMPARE(m_subject->previousSimState, (int)(m_sim->simStatus));

    doAndVerifyStateChange(SIM::UnknownStatus);

    SIM::SIMStatus newState = SIM::NotReady;

    // test error case
    m_subject->simStatusComplete(newState, SIMErrorBusCommunication);
    QCOMPARE(m_subject->previousSimState, (int)(m_sim->simStatus));

    // test ok case
    m_subject->simStatusComplete(newState, SIMErrorNone);
    QCOMPARE(m_subject->previousSimState, (int)newState);
}

void Ut_PinCodeQueryBusinessLogic::testUiControls()
{
    // enable UI operations
    m_subject->initialized = true;

    // check button enability
    doAndVerifyStateChange(SIM::PINRequired);
    QCOMPARE(m_subject->uiPin.isNull(), false);
    DuiButton *enter = m_subject->uiPin->getEnterBtn();
    DuiButton *cancel = m_subject->uiPin->getCancelBtn();

    // test enter button enablity
    QString pin("12");
    m_subject->uiPin->getCodeEntry()->setText(pin);
    QCOMPARE(enter->isEnabled(), false);
    QCOMPARE(cancel->isEnabled(), true);

    m_subject->uiPin->getCodeEntry()->setText(pin+pin);
    QCOMPARE(enter->isEnabled(), true);
    QCOMPARE(cancel->isEnabled(), true);

    // clean UI
    emitButtonReleased("cancelButton");

    doAndVerifyStateChange(SIM::Ok);
    m_subject->changePinCode();

    QCOMPARE(m_subject->uiPin.isNull(), false);
    enter = m_subject->uiPin.data()->getEnterBtn();
    cancel = m_subject->uiPin.data()->getCancelBtn();
    QCOMPARE(enter->isEnabled(), false);
    QCOMPARE(cancel->isEnabled(), true);

    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");

    QCOMPARE(enter->isEnabled(), false);
    QCOMPARE(cancel->isEnabled(), false);

    m_subject->uiPin->getCodeEntry()->setText(pin);
    QCOMPARE(enter->isEnabled(), false);
    QCOMPARE(cancel->isEnabled(), false);

    m_subject->uiPin->getCodeEntry()->setText(pin+pin);
    QCOMPARE(enter->isEnabled(), true);
    QCOMPARE(cancel->isEnabled(), false);
}

void Ut_PinCodeQueryBusinessLogic::testFailedLaunchPinQuery()
{
    m_dbus->queryDoneOk = false;

    // Test no start
    doAndVerifyStateChange(SIM::NotReady);
    bool resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, false);
    QCOMPARE(m_dbus->queryDoneOk, false);

    doAndVerifyStateChange(SIM::PINRequired);

    // test failed start
    m_simSec->error = SIMErrorNoSIM;
    m_simSec->success = false;
    m_dbus->queryDoneOk = true;

    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));
    resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, false);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMCardNotInserted);
    QCOMPARE(m_dbus->queryDoneOk, false);
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
}

void Ut_PinCodeQueryBusinessLogic::testLaunchPinQuery()
{
    m_dbus->queryDoneOk = false;

    // Test start
    doAndVerifyStateChange(SIM::PINRequired);
    bool resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, true);
    QCOMPARE(m_dbus->queryDoneOk, false);

    // Test nok signal
    doAndVerifyPinNok(QString("4321"), PINCodeIncorrectSettings);
    emitButtonReleased("cancelButton");
    QCOMPARE(m_dbus->queryDoneOk, false);

    // Test ok signal
    resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, true);
    QCOMPARE(m_dbus->queryDoneOk, false);

    doAndVerifyPinOk();
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
    QCOMPARE(m_dbus->queryDoneOk, true);
}

void Ut_PinCodeQueryBusinessLogic::testEnablePinQuery()
{
    m_simSec->pinAttempts = 4;
    m_simSec->pin = QString("1234");
    m_simSec->queryState = SIMSecurity::Enabled;
    m_subject->initialized = false;

    // test state query
    m_subject->pinQueryState(SIMSecurity::PIN);
    QCOMPARE(m_subject->uiPin.isNull(), true);
    QCOMPARE(m_dbus->state, m_simSec->queryState);

    // test state set, cancel
    m_subject->enablePinQueryRequested(false);
    QCOMPARE(m_subject->uiPin.isNull(), false);
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    emitButtonReleased("cancelButton");
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);

   QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));

    // test state set, wrong pin
    m_subject->enablePinQueryRequested(false);
    QCOMPARE(m_subject->uiPin.isNull(), false);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    m_subject->uiPin->getCodeEntry()->setText(QString("1111"));
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeIncorrectSettings);

    // test state set, correct pin
    m_subject->enablePinQueryRequested(false);
    QCOMPARE(m_subject->uiPin.isNull(), false);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
}

void Ut_PinCodeQueryBusinessLogic::testChangePinCode()
{
    doAndVerifyStateChange(SIM::NotReady);
    m_subject->changePinCode();
    QCOMPARE(m_subject->uiPin.isNull(), true);

    doAndVerifyStateChange(SIM::Ok);
    m_subject->changePinCode();
    QCOMPARE(m_subject->uiPin.isNull(), false);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));
    QString newPin("1111");

    // test wrong pin
    m_subject->uiPin->getCodeEntry()->setText(newPin);
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeIncorrectSettings);

    // test with correct PIN...
    m_subject->changePinCode();
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");

    // ...wrong new PIN
    m_subject->uiPin->getCodeEntry()->setText(newPin);
    emitButtonReleased("enterButton");
    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");
    QCOMPARE((m_simSec->pin == newPin), false);

    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodesDoNotMatch);

    // ...correct new PIN
    m_subject->uiPin->getCodeEntry()->setText(newPin);
    emitButtonReleased("enterButton");
    m_subject->uiPin->getCodeEntry()->setText(newPin);
    emitButtonReleased("enterButton");
    QCOMPARE(m_simSec->pin, newPin);
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);

    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeChanged);
}

void Ut_PinCodeQueryBusinessLogic::testPinToPuk()
{
    QString wrongPin(QString("7777"));
    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));

    m_subject->initialized = true;
    m_simSec->pin = QString("4321");
    m_simSec->pinAttempts = 4;

    doAndVerifyStateChange(SIM::PINRequired);
    QCOMPARE(m_subject->uiPin.isNull(), false);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    // test wrong pin #1
    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    m_simSec->pinAttempts = 3;
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeIncorrectSettings);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    // test wrong pin #2
    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    m_simSec->pinAttempts = 2;
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeIncorrect2AttemptsLeft);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    // test wrong pin #3
    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    m_simSec->pinAttempts = 1;
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeIncorrect1AttemptLeft);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    // last try...
    m_simSec->success = false;
    m_simSec->error = SIMErrorPINDisabled;
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 0);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    // init
    m_simSec->success = true;
    m_simSec->error = SIMErrorNone;

    // puk required...
    doAndVerifyStateChange(SIM::PUKRequired);
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeIncorrectPUKCodeRequired);

    // test limitations
    m_simSec->pukAttempts = 10;
    while(0 <= m_simSec->pukAttempts) {
        m_subject->uiPin->getCodeEntry()->setText(wrongPin);
        emitButtonReleased("enterButton");
        QCOMPARE(m_subject->uiPin->isVisible(), true);
        QCOMPARE(spy.count(), 1);
        arguments = spy.takeFirst();
        QVERIFY(arguments.at(0).toString() == PUKCodeIncorrect);
        QCOMPARE(m_subject->uiPin->isVisible(), true);
        m_simSec->pukAttempts--;
    }
    // ..and the last one
    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    m_simSec->pukAttempts = 10;
    m_subject->uiPin->getCodeEntry()->setText(m_simSec->puk);
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(m_simSec->pin, QString("1234")); // default new PIN

    // ...and new PIN
    doAndVerifyStateChange(SIM::Ok);
    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    emitButtonReleased("enterButton");
    m_subject->uiPin->getCodeEntry()->setText(wrongPin);
    emitButtonReleased("enterButton");
    QCOMPARE(m_simSec->pin, wrongPin);
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);

    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == PINCodeChanged);
}

void Ut_PinCodeQueryBusinessLogic::testSimLock()
{
    m_subject->initialized = true;
    m_subject->queryLaunch = false;
    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));

    doAndVerifyStateChange(SIM::SIMLockRejected);
    QCOMPARE(m_subject->uiPin.isNull(), false);
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMLocked);

    // test buttons enablity
    DuiButton *enter = m_subject->uiPin->getEnterBtn();
    DuiButton *cancel = m_subject->uiPin->getCancelBtn();
    QCOMPARE(enter->isEnabled(), true);
    QCOMPARE(cancel->isEnabled(), true);

    m_subject->uiPin->getCodeEntry()->setText(QString("1"));
    QCOMPARE(enter->isEnabled(), true);
    QCOMPARE(cancel->isEnabled(), true);

    // test wrong code
    m_subject->uiPin->getCodeEntry()->setText(QString("111111111111"));
    QCOMPARE(enter->isEnabled(), true);
    QCOMPARE(cancel->isEnabled(), true);
    emitButtonReleased("enterButton");

    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMLockIncorrect);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    // test cancel
    m_subject->uiPin->getCodeEntry()->setText(m_simLock->code);
    emitButtonReleased("cancelButton");
    QCOMPARE(m_subject->uiPin->isVisible(), false);

    bool resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, true);
    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMLocked);

    // test enter
    m_subject->uiPin->getCodeEntry()->setText(m_simLock->code);
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    QCOMPARE(spy.count(), 1);
    arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMUnlocked);
}

void Ut_PinCodeQueryBusinessLogic::testErrorAtPINQuery()
{
    m_simSec->pinAttempts = 2;
    m_subject->initialized = true;

    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));
    doAndVerifyStateChange(SIM::PINRequired);
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    // no notes shown though there are only 2 attempts left
    QCOMPARE(spy.count(), 0);

    // SIMCardNotInserted
    m_simSec->success = false;
    m_simSec->error = SIMErrorNoSIM;
    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMCardNotInserted);
    // test UI still open
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    emitButtonReleased("cancelButton");
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
}

void Ut_PinCodeQueryBusinessLogic::testErrorAtPUKQuery()
{
    m_subject->initialized = true;

    QSignalSpy spy(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));
    doAndVerifyStateChange(SIM::PUKRequired);
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    // no note shown for incorrect PIN
    QCOMPARE(spy.count(), 0);

    m_simSec->success = false;
    m_simSec->error = SIMErrorSIMRejected;
    m_subject->uiPin->getCodeEntry()->setText(m_simSec->puk);
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMCardRejected);
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
}

void Ut_PinCodeQueryBusinessLogic::testTechnicalProblem()
{
    m_simSec->success = false;
    m_simSec->error = SIMErrorNone;
    m_dbus->queryDoneOk = true;

    QSignalSpy spy(m_subject, SIGNAL(showConfirmation(const QString &, const QString &)));
    doAndVerifyStateChange(SIM::PINRequired);
    bool resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, true);
    QCOMPARE(m_subject->queryLaunch, true);
    QCOMPARE(m_subject->uiPin->isVisible(), true);

    m_simSec->error = SIMErrorServiceBusy;
    m_subject->uiPin->getCodeEntry()->setText(m_simSec->pin);
    emitButtonReleased("enterButton");
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toString() == TechnicalProblem);
    QVERIFY(arguments.at(1).toString() == trid("qtn_cell_continue", "Continue"));
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(m_dbus->queryDoneOk, true);
    m_dbus->queryDoneOk = false;

    // interaction:
    m_subject->cancelQuery();
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
    QCOMPARE(m_dbus->queryDoneOk, false);
}

void Ut_PinCodeQueryBusinessLogic::testSIMLockRetry()
{
    m_simLock->error = SIMLockErrorNone;
    m_dbus->queryDoneOk = true;

    QSignalSpy spyNote(m_subject, SIGNAL(showNotification(QString, NotificationType::Type)));
    doAndVerifyStateChange(SIM::SIMLockRejected);
    bool resp = m_subject->launchPinQuery(SIMSecurity::PIN);
    QCOMPARE(resp, true);
    QCOMPARE(m_subject->queryLaunch, true);
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(spyNote.count(), 1);
    QList<QVariant> arguments = spyNote.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMLocked);

    QSignalSpy spyConf(m_subject, SIGNAL(showConfirmation(const QString &, const QString &)));

    m_simLock->error = SIMLockErrorTimerNotElapsed;
    m_subject->uiPin->getCodeEntry()->setText(m_simLock->code);
    emitButtonReleased("enterButton");
    QCOMPARE(m_subject->uiPin->isVisible(), true);
    QCOMPARE(m_subject->queryLaunch, true);
    QCOMPARE(spyConf.count(), 1);
    arguments = spyConf.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMLockTooFast);
    QVERIFY(arguments.at(1).toString() == trid("qtn_cell_try_again", "Try again"));
    emitButtonReleased("cancelButton");
    QCOMPARE(m_subject->simLockCode, m_simLock->code);
    QCOMPARE(m_subject->subState, PinCodeQueryBusinessLogic::SubNothing);
    QCOMPARE(m_subject->uiPin->isVisible(), false);
    QCOMPARE(m_dbus->queryDoneOk, false);
    m_dbus->queryDoneOk = true;

    // interaction
    m_simLock->error = SIMLockErrorNone;
    m_subject->resendSimLockCode();
    QCOMPARE(spyNote.count(), 1);
    arguments = spyNote.takeFirst();
    QVERIFY(arguments.at(0).toString() == SIMUnlocked);
    QCOMPARE(m_subject->simLockCode.length(), 0);
    QCOMPARE(m_dbus->queryDoneOk, true);
}


QTEST_APPLESS_MAIN(Ut_PinCodeQueryBusinessLogic)